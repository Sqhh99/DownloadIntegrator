#include "CoverExtractor.h"
#include <QNetworkRequest>
#include <QStandardPaths>
#include <QDir>
#include <QCryptographicHash>
#include <QDebug>
#include <QBuffer>
#include <QImageReader>
#include <QApplication>
#include <algorithm>
#include <vector>

CoverExtractor::CoverExtractor(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_callback(nullptr)
{
}

CoverExtractor::~CoverExtractor()
{
}

void CoverExtractor::extractCoverFromTrainerImage(const QString& imageUrl, 
                                                 std::function<void(const QPixmap&, bool)> callback)
{
    m_callback = callback;
    
    QNetworkRequest request(imageUrl);
    request.setHeader(QNetworkRequest::UserAgentHeader, 
                     "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36");
    
    QNetworkReply* reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, &CoverExtractor::onImageDownloaded);
}

void CoverExtractor::onImageDownloaded()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    
    if (!reply || !m_callback) {
        if (reply) reply->deleteLater();
        return;
    }
    
    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "图片下载失败：" << reply->errorString();
        m_callback(QPixmap(), false);
        reply->deleteLater();
        return;
    }
    
    QByteArray imageData = reply->readAll();
    reply->deleteLater();
    
    // 从下载的数据创建QPixmap
    QPixmap originalPixmap;
    if (!originalPixmap.loadFromData(imageData)) {
        qDebug() << "无法加载图片数据";
        m_callback(QPixmap(), false);
        return;
    }
    
    qDebug() << "图片下载成功，尺寸：" << originalPixmap.size();
    
    // 使用形状分析方法处理图片
    QPixmap coverPixmap = processTrainerImage(originalPixmap);
    
    if (!coverPixmap.isNull()) {
        qDebug() << "封面提取成功，尺寸：" << coverPixmap.size();
        m_callback(coverPixmap, true);
    } else {
        qDebug() << "封面提取失败";
        m_callback(QPixmap(), false);
    }
}

QPixmap CoverExtractor::processTrainerImage(const QPixmap& originalImage)
{
    if (originalImage.isNull()) {
        return QPixmap();
    }
    
    // 转换为OpenCV Mat
    cv::Mat image = qPixmapToMat(originalImage);
    if (image.empty()) {
        qDebug() << "图片转换失败";
        return QPixmap();
    }
    
    // 使用形状分析提取封面
    cv::Mat cover = extractCoverByShapeAnalysis(image);
    
    if (cover.empty()) {
        qDebug() << "形状分析失败，尝试备用方案";
        return QPixmap();
    }
    
    // 转换回QPixmap
    return matToQPixmap(cover);
}

cv::Mat CoverExtractor::extractCoverByShapeAnalysis(const cv::Mat& image)
{
    try {
        int height = image.rows;
        int width = image.cols;
        qDebug() << QString("开始形状分析，图片尺寸: %1 x %2").arg(width).arg(height);
        
        // 智能缩放处理
        cv::Mat processImage = image;
        float scale = 1.0f;
        const int MAX_PROCESS_WIDTH = 600;
        if (width > MAX_PROCESS_WIDTH) {
            scale = static_cast<float>(MAX_PROCESS_WIDTH) / width;
            cv::resize(image, processImage, cv::Size(), scale, scale, cv::INTER_AREA);
        }
        
        // 关键优化：封面通常在左侧30-35%的区域内，限制搜索范围避免包含右侧选项
        int roiWidth = static_cast<int>(processImage.cols * 0.38);  // 最大38%宽度
        int roiHeight = static_cast<int>(processImage.rows * 0.9);
        cv::Rect roiRect(0, 0, roiWidth, roiHeight);
        cv::Mat roi = processImage(roiRect);
        
        qDebug() << QString("分析区域尺寸: %1 x %2 (限制在左侧38%%)").arg(roiWidth).arg(roiHeight);
        
        // 首先尝试检测封面的右边界（找到封面和选项区的分界线）
        int coverRightBound = detectCoverRightBoundary(roi);
        if (coverRightBound > 0 && coverRightBound < roiWidth * 0.95) {
            qDebug() << QString("检测到封面右边界: %1").arg(coverRightBound);
            // 缩小ROI到检测到的边界
            roiWidth = coverRightBound + 5;  // 留一点边距
            roiRect = cv::Rect(0, 0, roiWidth, roiHeight);
            roi = processImage(roiRect);
        }
        
        // 查找封面候选区域
        std::vector<CoverCandidate> candidates = findCoverCandidates(roi);
        
        // 过滤掉明显不是封面的候选（宽度过大的）
        candidates.erase(
            std::remove_if(candidates.begin(), candidates.end(),
                [roiWidth](const CoverCandidate& c) {
                    // 封面宽度不应该超过ROI宽度的90%
                    return c.w > roiWidth * 0.92;
                }),
            candidates.end()
        );
        
        // 如果主要方法失败，尝试颜色分割
        if (candidates.empty()) {
            qDebug() << "轮廓检测失败，尝试颜色分割方法";
            candidates = findCoverByColorSegmentation(roi);
            
            // 同样过滤
            candidates.erase(
                std::remove_if(candidates.begin(), candidates.end(),
                    [roiWidth](const CoverCandidate& c) {
                        return c.w > roiWidth * 0.92;
                    }),
                candidates.end()
            );
        }
        
        if (!candidates.empty()) {
            // 选择最佳候选，优先选择竖版且不太宽的
            auto best_it = std::max_element(candidates.begin(), candidates.end(),
                [](const CoverCandidate& a, const CoverCandidate& b) {
                    double aspectA = static_cast<double>(a.w) / a.h;
                    double aspectB = static_cast<double>(b.w) / b.h;
                    
                    // 竖版封面加分（长宽比 0.6-0.85 最佳）
                    double aspectBonusA = (aspectA >= 0.55 && aspectA <= 0.9) ? 1.5 : 1.0;
                    double aspectBonusB = (aspectB >= 0.55 && aspectB <= 0.9) ? 1.5 : 1.0;
                    
                    double scoreA = a.area * a.quality * aspectBonusA;
                    double scoreB = b.area * b.quality * aspectBonusB;
                    return scoreA < scoreB;
                });
            
            const CoverCandidate& best = *best_it;
            qDebug() << QString("选择最佳候选: 位置(%1,%2) 尺寸(%3x%4) 长宽比:%5")
                        .arg(best.x).arg(best.y).arg(best.w).arg(best.h)
                        .arg(static_cast<double>(best.w) / best.h, 0, 'f', 2);
            
            // 提取封面
            cv::Mat cover;
            if (scale < 1.0f) {
                // 映射回原图
                int origRoiWidth = static_cast<int>(width * 0.38);
                if (coverRightBound > 0) {
                    origRoiWidth = static_cast<int>((coverRightBound + 5) / scale);
                }
                int origRoiHeight = static_cast<int>(height * 0.9);
                
                cv::Rect coverRect(
                    static_cast<int>(best.x / scale),
                    static_cast<int>(best.y / scale),
                    static_cast<int>(best.w / scale),
                    static_cast<int>(best.h / scale)
                );
                
                coverRect.x = std::max(0, std::min(coverRect.x, origRoiWidth - 1));
                coverRect.y = std::max(0, std::min(coverRect.y, origRoiHeight - 1));
                coverRect.width = std::min(coverRect.width, origRoiWidth - coverRect.x);
                coverRect.height = std::min(coverRect.height, origRoiHeight - coverRect.y);
                
                cv::Rect origRoiRect(0, 0, 
                    std::min(origRoiWidth, width), 
                    std::min(origRoiHeight, height));
                cv::Mat origRoi = image(origRoiRect);
                cover = origRoi(coverRect).clone();
            } else {
                cv::Rect coverRect(best.x, best.y, best.w, best.h);
                cover = roi(coverRect).clone();
            }
            
            // 边框优化
            if (cover.cols > 80 && cover.rows > 100) {
                cv::Mat optimizedCover = removeCoverBorders(cover);
                return optimizedCover.empty() ? cover : optimizedCover;
            }
            
            return cover;
        } else {
            qDebug() << "未找到候选区域，使用备用方案";
            return extractFallbackByPosition(roi);
        }
        
    } catch (const std::exception& e) {
        qDebug() << "形状分析异常：" << e.what();
        return cv::Mat();
    }
}

// 检测封面的右边界（找到封面和选项列表的分界线）
int CoverExtractor::detectCoverRightBoundary(const cv::Mat& roi)
{
    try {
        int height = roi.rows;
        int width = roi.cols;
        
        // 转换为灰度
        cv::Mat gray;
        cv::cvtColor(roi, gray, cv::COLOR_BGR2GRAY);
        
        // 方法1：检测垂直边缘（封面右边通常有明显的垂直边界）
        cv::Mat sobelX;
        cv::Sobel(gray, sobelX, CV_16S, 1, 0, 3);
        cv::Mat absSobelX;
        cv::convertScaleAbs(sobelX, absSobelX);
        
        // 统计每列的垂直边缘强度
        std::vector<double> colEdgeStrength(width, 0);
        for (int x = 0; x < width; ++x) {
            cv::Scalar sum = cv::sum(absSobelX.col(x));
            colEdgeStrength[x] = sum[0] / height;
        }
        
        // 在20%-80%的范围内寻找强垂直边缘
        int searchStart = static_cast<int>(width * 0.2);
        int searchEnd = static_cast<int>(width * 0.85);
        
        double maxStrength = 0;
        int maxPos = -1;
        
        for (int x = searchStart; x < searchEnd; ++x) {
            // 寻找局部最大值（边缘）
            if (colEdgeStrength[x] > maxStrength && colEdgeStrength[x] > 30) {
                // 检查这个位置是否是一条连续的垂直线
                // 通过检查上下几行的边缘强度一致性
                bool isVerticalLine = true;
                double avgStrength = colEdgeStrength[x];
                
                // 简单检查：如果这一列的边缘强度明显高于平均值
                double avgAllCols = 0;
                for (int i = searchStart; i < searchEnd; ++i) {
                    avgAllCols += colEdgeStrength[i];
                }
                avgAllCols /= (searchEnd - searchStart);
                
                if (colEdgeStrength[x] > avgAllCols * 1.5) {
                    maxStrength = colEdgeStrength[x];
                    maxPos = x;
                }
            }
        }
        
        // 方法2：检测颜色变化（封面区域颜色丰富，选项区域颜色单一）
        if (maxPos < 0) {
            std::vector<double> colColorVariance(width, 0);
            
            for (int x = 0; x < width; ++x) {
                cv::Mat col = roi.col(x);
                cv::Scalar mean, stddev;
                cv::meanStdDev(col, mean, stddev);
                // 颜色方差 = 各通道标准差之和
                colColorVariance[x] = stddev[0] + stddev[1] + stddev[2];
            }
            
            // 找到颜色方差突然下降的位置（从封面进入选项区）
            for (int x = searchStart; x < searchEnd - 10; ++x) {
                double leftVariance = 0, rightVariance = 0;
                for (int i = 0; i < 10; ++i) {
                    leftVariance += colColorVariance[x - 5 + i];
                    rightVariance += colColorVariance[x + 5 + i];
                }
                leftVariance /= 10;
                rightVariance /= 10;
                
                // 如果左边颜色丰富，右边颜色单一
                if (leftVariance > rightVariance * 1.8 && leftVariance > 20) {
                    maxPos = x;
                    qDebug() << QString("通过颜色变化检测到边界: %1 (左方差:%2, 右方差:%3)")
                                .arg(x).arg(leftVariance, 0, 'f', 1).arg(rightVariance, 0, 'f', 1);
                    break;
                }
            }
        }
        
        return maxPos;
        
    } catch (const std::exception& e) {
        qDebug() << "边界检测出错：" << e.what();
        return -1;
    }
}

std::vector<CoverCandidate> CoverExtractor::findCoverCandidates(const cv::Mat& roi)
{
    std::vector<CoverCandidate> candidates;
    
    try {
        int height = roi.rows;
        int width = roi.cols;
        
        // 转换为灰度
        cv::Mat gray;
        cv::cvtColor(roi, gray, cv::COLOR_BGR2GRAY);
        
        // 应用鲁棒的边缘检测
        cv::Mat edges = applyRobustEdgeDetection(gray);
        
        // 查找轮廓
        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(edges, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
        
        qDebug() << QString("找到 %1 个轮廓").arg(contours.size());
        
        // 动态设置最小面积阈值（根据图片大小调整）
        double minArea = std::max(3000.0, (width * height) * 0.01);  // 降低阈值
        
        for (size_t i = 0; i < contours.size(); ++i) {
            double area = cv::contourArea(contours[i]);
            
            if (area > minArea) {
                cv::Rect boundRect = cv::boundingRect(contours[i]);
                double aspectRatio = static_cast<double>(boundRect.width) / boundRect.height;
                
                // 放宽长宽比限制，适应更多封面类型
                // 典型游戏封面长宽比在 0.6-0.85 之间（竖版）
                bool validAspectRatio = (aspectRatio > 0.45 && aspectRatio < 1.3);
                bool validSize = (boundRect.width > 60 && boundRect.height > 80);
                bool validPosition = (boundRect.x >= 0 && boundRect.y >= 0);
                
                if (validAspectRatio && validSize && validPosition) {
                    cv::Mat coverRegion = roi(boundRect);
                    double quality = calculateRegionQuality(coverRegion);
                    
                    // 位置权重：偏左上的区域加分
                    double positionBonus = 1.0 + (1.0 - static_cast<double>(boundRect.x) / width) * 0.3;
                    quality *= positionBonus;
                    
                    candidates.emplace_back(boundRect.x, boundRect.y, 
                                          boundRect.width, boundRect.height,
                                          area, quality, static_cast<int>(i));
                    
                    qDebug() << QString("候选区域 %1: 位置(%2,%3) 尺寸(%4x%5) 面积:%6 质量:%7")
                                .arg(i).arg(boundRect.x).arg(boundRect.y)
                                .arg(boundRect.width).arg(boundRect.height)
                                .arg(area).arg(quality, 0, 'f', 2);
                    
                    // 如果找到足够好的候选，提前退出
                    if (area > minArea * 5 && quality > 1.5) {
                        qDebug() << "找到优质候选区域，提前结束搜索";
                        break;
                    }
                }
            }
        }
        
        // 降级策略：如果没有找到合适的候选，放宽条件重试
        if (candidates.empty() && !contours.empty()) {
            qDebug() << "使用降级策略：放宽条件搜索";
            
            for (size_t i = 0; i < contours.size(); ++i) {
                double area = cv::contourArea(contours[i]);
                
                if (area > 1500) {  // 更低的面积阈值
                    cv::Rect boundRect = cv::boundingRect(contours[i]);
                    
                    if (boundRect.width > 40 && boundRect.height > 50) {
                        cv::Mat coverRegion = roi(boundRect);
                        double quality = calculateRegionQuality(coverRegion);
                        
                        candidates.emplace_back(boundRect.x, boundRect.y,
                                              boundRect.width, boundRect.height,
                                              area, quality, static_cast<int>(i));
                    }
                }
            }
        }
        
    } catch (const std::exception& e) {
        qDebug() << "寻找候选区域时出错：" << e.what();
    }
    
    return candidates;
}

// 使用颜色分割方法查找封面候选区域
std::vector<CoverCandidate> CoverExtractor::findCoverByColorSegmentation(const cv::Mat& roi)
{
    std::vector<CoverCandidate> candidates;
    
    try {
        int height = roi.rows;
        int width = roi.cols;
        
        // 转换到 HSV 色彩空间
        cv::Mat hsv;
        cv::cvtColor(roi, hsv, cv::COLOR_BGR2HSV);
        
        // 分析背景颜色（通常是深色或纯色）
        // 修改器界面背景通常是深灰色或黑色
        cv::Mat mask;
        
        // 方法1：检测非背景区域（非深色区域）
        cv::Mat grayRoi;
        cv::cvtColor(roi, grayRoi, cv::COLOR_BGR2GRAY);
        
        // 使用自适应阈值分割
        cv::Mat binary;
        cv::adaptiveThreshold(grayRoi, binary, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, 
                             cv::THRESH_BINARY, 15, -5);
        
        // 形态学操作：闭运算填充空洞
        cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5, 5));
        cv::morphologyEx(binary, binary, cv::MORPH_CLOSE, kernel);
        cv::morphologyEx(binary, binary, cv::MORPH_OPEN, kernel);
        
        // 查找轮廓
        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(binary, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
        
        qDebug() << QString("颜色分割找到 %1 个区域").arg(contours.size());
        
        double minArea = std::max(2000.0, (width * height) * 0.008);
        
        for (size_t i = 0; i < contours.size(); ++i) {
            double area = cv::contourArea(contours[i]);
            
            if (area > minArea) {
                cv::Rect boundRect = cv::boundingRect(contours[i]);
                double aspectRatio = static_cast<double>(boundRect.width) / boundRect.height;
                
                // 检查是否符合封面特征
                if (aspectRatio > 0.4 && aspectRatio < 1.4 &&
                    boundRect.width > 50 && boundRect.height > 70) {
                    
                    cv::Mat coverRegion = roi(boundRect);
                    double quality = calculateRegionQuality(coverRegion);
                    
                    // 颜色丰富度检查
                    cv::Scalar meanColor = cv::mean(coverRegion);
                    double colorVariance = std::abs(meanColor[0] - meanColor[1]) + 
                                          std::abs(meanColor[1] - meanColor[2]);
                    
                    // 封面通常有较丰富的颜色
                    if (colorVariance > 5 || quality > 0.5) {
                        candidates.emplace_back(boundRect.x, boundRect.y,
                                              boundRect.width, boundRect.height,
                                              area, quality, static_cast<int>(i));
                    }
                }
            }
        }
        
    } catch (const std::exception& e) {
        qDebug() << "颜色分割出错：" << e.what();
    }
    
    return candidates;
}

cv::Mat CoverExtractor::applyRobustEdgeDetection(const cv::Mat& gray)
{
    cv::Mat edges;
    
    try {
        // 高斯模糊以减少噪声
        cv::Mat blurred;
        cv::GaussianBlur(gray, blurred, cv::Size(3, 3), 0);
        
        // 优化：使用单次Canny边缘检测，而不是三次（提升3倍速度）
        cv::Canny(blurred, edges, 50, 120);
        
        // 简化的形态学操作（提升速度）
        cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
        cv::morphologyEx(edges, edges, cv::MORPH_CLOSE, kernel);
        
    } catch (const std::exception& e) {
        qDebug() << "边缘检测出错：" << e.what();
        return cv::Mat();
    }
    
    return edges;
}

double CoverExtractor::calculateRegionQuality(const cv::Mat& region)
{
    try {
        if (region.empty()) {
            return 0.0;
        }
        
        int height = region.rows;
        int width = region.cols;
        
        // 1. 尺寸评分 - 封面通常有一定大小
        double sizeScore = std::min(2.0, (width * height) / 40000.0);
        
        // 2. 长宽比评分 - 游戏封面通常是竖版 (0.6-0.85)
        double aspectRatio = static_cast<double>(width) / height;
        double aspectScore = 0.5;
        if (aspectRatio >= 0.55 && aspectRatio <= 0.9) {
            aspectScore = 1.5;  // 理想的竖版封面
        } else if (aspectRatio >= 0.45 && aspectRatio <= 1.1) {
            aspectScore = 1.0;  // 可接受的比例
        }
        
        // 3. 纹理复杂度评分 - 封面通常有丰富的细节
        cv::Mat gray;
        cv::cvtColor(region, gray, cv::COLOR_BGR2GRAY);
        
        cv::Scalar meanScalar, stdScalar;
        cv::meanStdDev(gray, meanScalar, stdScalar);
        double textureScore = std::min(2.0, stdScalar[0] / 25.0);
        
        // 4. 亮度评分 - 封面通常不会太暗或太亮
        double meanBrightness = meanScalar[0];
        double brightnessScore = 0.3;
        if (meanBrightness > 40 && meanBrightness < 200) {
            brightnessScore = 1.0;
        } else if (meanBrightness > 25 && meanBrightness < 230) {
            brightnessScore = 0.7;
        }
        
        // 5. 颜色丰富度评分（快速计算）
        cv::Scalar meanColor = cv::mean(region);
        double colorVariance = std::abs(meanColor[0] - meanColor[1]) + 
                              std::abs(meanColor[1] - meanColor[2]) +
                              std::abs(meanColor[0] - meanColor[2]);
        double colorScore = std::min(1.5, colorVariance / 30.0);
        
        // 综合得分
        double qualityScore = (
            sizeScore * 0.3 +      // 尺寸权重
            aspectScore * 0.25 +   // 长宽比权重
            textureScore * 0.25 +  // 纹理权重
            brightnessScore * 0.1 + // 亮度权重
            colorScore * 0.1       // 颜色权重
        );
        
        return std::max(0.1, std::min(qualityScore, 10.0));
        
    } catch (const std::exception& e) {
        qDebug() << "质量评分计算错误：" << e.what();
        return 0.5;
    }
}

cv::Mat CoverExtractor::extractFallbackByPosition(const cv::Mat& roi)
{
    try {
        int height = roi.rows;
        int width = roi.cols;
        
        // 更多的备选位置，覆盖不同的修改器界面布局
        std::vector<cv::Rect> positions = {
            // 标准左上角位置
            cv::Rect(static_cast<int>(width * 0.02), static_cast<int>(height * 0.08), 
                    static_cast<int>(width * 0.4), static_cast<int>(height * 0.65)),
            // 稍微偏右的位置
            cv::Rect(static_cast<int>(width * 0.05), static_cast<int>(height * 0.1), 
                    static_cast<int>(width * 0.35), static_cast<int>(height * 0.55)),
            // 更靠近边缘
            cv::Rect(static_cast<int>(width * 0.01), static_cast<int>(height * 0.05), 
                    static_cast<int>(width * 0.45), static_cast<int>(height * 0.7)),
            // 居中偏左
            cv::Rect(static_cast<int>(width * 0.08), static_cast<int>(height * 0.12), 
                    static_cast<int>(width * 0.32), static_cast<int>(height * 0.5)),
            // 更大范围
            cv::Rect(static_cast<int>(width * 0.0), static_cast<int>(height * 0.02), 
                    static_cast<int>(width * 0.5), static_cast<int>(height * 0.75))
        };
        
        cv::Mat bestCover;
        double bestScore = 0;
        
        for (size_t i = 0; i < positions.size(); ++i) {
            cv::Rect pos = positions[i];
            
            // 确保不超出边界
            pos.x = std::max(0, std::min(pos.x, width - 1));
            pos.y = std::max(0, std::min(pos.y, height - 1));
            pos.width = std::min(pos.width, width - pos.x);
            pos.height = std::min(pos.height, height - pos.y);
            
            if (pos.width > 40 && pos.height > 60) {
                cv::Mat cover = roi(pos);
                double score = calculateRegionQuality(cover);
                
                qDebug() << QString("固定位置 %1: 位置(%2,%3) 尺寸(%4x%5) 质量%6")
                            .arg(i+1).arg(pos.x).arg(pos.y)
                            .arg(pos.width).arg(pos.height).arg(score, 0, 'f', 2);
                
                if (score > bestScore) {
                    bestScore = score;
                    bestCover = cover.clone();
                }
            }
        }
        
        return bestScore > 0.5 ? bestCover : cv::Mat();
        
    } catch (const std::exception& e) {
        qDebug() << "固定位置提取失败：" << e.what();
        return cv::Mat();
    }
}

cv::Mat CoverExtractor::removeCoverBorders(const cv::Mat& coverImage)
{
    if (coverImage.empty()) {
        return cv::Mat();
    }
    
    try {
        // 转换为灰度
        cv::Mat gray;
        cv::cvtColor(coverImage, gray, cv::COLOR_BGR2GRAY);
        
        // 边缘检测
        cv::Mat edges;
        cv::Canny(gray, edges, 30, 100);
        
        // 形态学操作
        cv::Mat kernel = cv::Mat::ones(3, 3, CV_8U);
        cv::morphologyEx(edges, edges, cv::MORPH_CLOSE, kernel);
        
        // 查找轮廓
        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(edges, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
        
        if (!contours.empty()) {
            // 找到最大轮廓
            auto largestContour = std::max_element(contours.begin(), contours.end(),
                [](const std::vector<cv::Point>& a, const std::vector<cv::Point>& b) {
                    return cv::contourArea(a) < cv::contourArea(b);
                });
            
            cv::Rect boundRect = cv::boundingRect(*largestContour);
            
            // 检查裁切区域是否合理
            int originalArea = coverImage.rows * coverImage.cols;
            int cropArea = boundRect.width * boundRect.height;
            
            if (cropArea > originalArea * 0.5 &&
                boundRect.width > coverImage.cols * 0.6 &&
                boundRect.height > coverImage.rows * 0.6) {
                
                // 添加小边距
                int margin = 3;
                boundRect.x = std::max(0, boundRect.x - margin);
                boundRect.y = std::max(0, boundRect.y - margin);
                boundRect.width = std::min(coverImage.cols - boundRect.x, boundRect.width + 2 * margin);
                boundRect.height = std::min(coverImage.rows - boundRect.y, boundRect.height + 2 * margin);
                
                return coverImage(boundRect).clone();
            }
        }
        
        return coverImage;
        
    } catch (const std::exception& e) {
        qDebug() << "边框移除失败：" << e.what();
        return coverImage;
    }
}

QPixmap CoverExtractor::matToQPixmap(const cv::Mat& mat)
{
    try {
        if (mat.empty()) {
            return QPixmap();
        }
        
        cv::Mat rgbMat;
        if (mat.channels() == 3) {
            cv::cvtColor(mat, rgbMat, cv::COLOR_BGR2RGB);
        } else if (mat.channels() == 4) {
            cv::cvtColor(mat, rgbMat, cv::COLOR_BGRA2RGBA);
        } else {
            cv::cvtColor(mat, rgbMat, cv::COLOR_GRAY2RGB);
        }
        
        QImage qimg(rgbMat.data, rgbMat.cols, rgbMat.rows, 
                   static_cast<int>(rgbMat.step), QImage::Format_RGB888);
        return QPixmap::fromImage(qimg);
        
    } catch (const std::exception& e) {
        qDebug() << "Mat转QPixmap失败：" << e.what();
        return QPixmap();
    }
}

cv::Mat CoverExtractor::qPixmapToMat(const QPixmap& pixmap)
{
    try {
        QImage qimg = pixmap.toImage();
        qimg = qimg.convertToFormat(QImage::Format_RGB888);
        
        cv::Mat mat(qimg.height(), qimg.width(), CV_8UC3, 
                   const_cast<uchar*>(qimg.bits()), 
                   static_cast<size_t>(qimg.bytesPerLine()));
        
        cv::Mat result;
        cv::cvtColor(mat, result, cv::COLOR_RGB2BGR);
        return result.clone();
        
    } catch (const std::exception& e) {
        qDebug() << "QPixmap转Mat失败：" << e.what();
        return cv::Mat();
    }
}

// 保持原有的静态方法接口
QPixmap CoverExtractor::extractCoverFromLocalImage(const QString& imagePath)
{
    QPixmap originalPixmap(imagePath);
    return processTrainerImage(originalPixmap);
}

QString CoverExtractor::getCacheDirectory()
{
    QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    QDir dir(cacheDir);
    if (!dir.exists("covers")) {
        dir.mkpath("covers");
    }
    return dir.absoluteFilePath("covers");
}

QPixmap CoverExtractor::getCachedCover(const QString& gameId)
{
    QString cacheDir = getCacheDirectory();
    QString cachedPath = QDir(cacheDir).absoluteFilePath(gameId + ".png");
    
    if (QFile::exists(cachedPath)) {
        QPixmap cached(cachedPath);
        if (!cached.isNull()) {
            return cached;
        }
    }
    
    return QPixmap();
}

bool CoverExtractor::saveCoverToCache(const QString& gameId, const QPixmap& cover)
{
    if (cover.isNull()) {
        return false;
    }
    
    QString cacheDir = getCacheDirectory();
    QString cachedPath = QDir(cacheDir).absoluteFilePath(gameId + ".png");
    
    return cover.save(cachedPath, "PNG");
}