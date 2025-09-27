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
        
        // 专注于左上角区域（游戏封面通常在此位置）
        int roiWidth = static_cast<int>(width * 0.4);
        int roiHeight = static_cast<int>(height * 0.8);
        cv::Rect roiRect(0, 0, roiWidth, roiHeight);
        cv::Mat roi = image(roiRect);
        
        qDebug() << QString("分析区域尺寸: %1 x %2").arg(roiWidth).arg(roiHeight);
        
        // 查找封面候选区域
        std::vector<CoverCandidate> candidates = findCoverCandidates(roi);
        
        if (!candidates.empty()) {
            // 按综合得分排序（面积 × 质量得分）
            std::sort(candidates.begin(), candidates.end(), 
                     [](const CoverCandidate& a, const CoverCandidate& b) {
                         return (a.area * a.quality) > (b.area * b.quality);
                     });
            
            const CoverCandidate& best = candidates[0];
            qDebug() << QString("选择最佳候选区域: 位置(%1,%2) 尺寸(%3x%4) 综合得分%5")
                        .arg(best.x).arg(best.y).arg(best.w).arg(best.h)
                        .arg(best.area * best.quality);
            
            // 提取封面区域
            cv::Rect coverRect(best.x, best.y, best.w, best.h);
            cv::Mat cover = roi(coverRect);
            
            // 进一步优化边框
            cv::Mat optimizedCover = removeCoverBorders(cover);
            
            return optimizedCover.empty() ? cover : optimizedCover;
        } else {
            qDebug() << "未找到符合条件的封面区域，尝试备用方案";
            return extractFallbackByPosition(roi);
        }
        
    } catch (const std::exception& e) {
        qDebug() << "形状分析异常：" << e.what();
        return cv::Mat();
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
        
        // 设置最小面积阈值（降低面积要求）
        double minArea = std::max(5000.0, (width * height) * 0.015);
        
        std::vector<QString> filteredReasons;
        
        for (size_t i = 0; i < contours.size(); ++i) {
            double area = cv::contourArea(contours[i]);
            
            if (area > minArea) {
                // 近似为多边形
                std::vector<cv::Point> approx;
                double epsilon = 0.02 * cv::arcLength(contours[i], true);
                cv::approxPolyDP(contours[i], approx, epsilon, true);
                
                // 至少是四边形
                if (approx.size() >= 4) {
                    cv::Rect boundRect = cv::boundingRect(contours[i]);
                    double aspectRatio = static_cast<double>(boundRect.width) / boundRect.height;
                    
                    bool passed = true;
                    QString reason;
                    
                    // 放宽长宽比要求（与Python一致）
                    if (!(aspectRatio > 0.5 && aspectRatio < 1.2)) {
                        reason += QString("长宽比%1不符合要求(0.5-1.2); ").arg(aspectRatio, 0, 'f', 2);
                        passed = false;
                    }
                    
                    // 放宽最小尺寸要求（与Python一致）
                    if (boundRect.width < 80 || boundRect.height < 120) {
                        reason += QString("尺寸太小(%1x%2，要求>80x120); ").arg(boundRect.width).arg(boundRect.height);
                        passed = false;
                    }
                    
                    // 放宽边缘距离要求
                    if (boundRect.x < 3 || boundRect.y < 3) {
                        reason += QString("太靠近边缘(x=%1, y=%2); ").arg(boundRect.x).arg(boundRect.y);
                        passed = false;
                    }
                    
                    if (passed) {
                        // 计算质量得分
                        cv::Mat coverRegion = roi(boundRect);
                        double quality = calculateRegionQuality(coverRegion);
                        
                        candidates.emplace_back(boundRect.x, boundRect.y, 
                                              boundRect.width, boundRect.height,
                                              area, quality, static_cast<int>(i));
                        
                        qDebug() << QString("✓ 候选区域 %1: 位置(%2,%3) 尺寸(%4x%5) 比例%6 质量%7")
                                    .arg(i).arg(boundRect.x).arg(boundRect.y)
                                    .arg(boundRect.width).arg(boundRect.height)
                                    .arg(aspectRatio, 0, 'f', 2).arg(quality, 0, 'f', 2);
                    } else {
                        filteredReasons.push_back(QString("区域 %1: %2").arg(i).arg(reason));
                    }
                } else {
                    filteredReasons.push_back(QString("区域 %1: 多边形顶点数%2<4").arg(i).arg(approx.size()));
                }
            } else {
                filteredReasons.push_back(QString("区域 %1: 面积%2<%3").arg(i).arg(area, 0, 'f', 0).arg(minArea, 0, 'f', 0));
            }
        }
        
        // 显示过滤信息
        if (!filteredReasons.empty()) {
            qDebug() << QString("过滤掉的区域: %1个").arg(filteredReasons.size());
            for (int i = 0; i < std::min(static_cast<int>(filteredReasons.size()), 5); ++i) {
                qDebug() << QString("  - %1").arg(filteredReasons[i]);
            }
            if (filteredReasons.size() > 5) {
                qDebug() << QString("  ... 还有%1个被过滤").arg(static_cast<int>(filteredReasons.size()) - 5);
            }
        }
        
        // 如果没有找到候选区域，尝试降级策略
        if (candidates.empty()) {
            qDebug() << "✖ 未找到符合条件的封面区域";
            qDebug() << QString("总轮廓数: %1").arg(contours.size());
            
            // 策略1: 降低要求
            qDebug() << "尝试降级处理策癥1: 降低要求...";
            std::vector<CoverCandidate> backupCandidates;
            
            for (size_t i = 0; i < contours.size(); ++i) {
                double area = cv::contourArea(contours[i]);
                // 进一步降低面积要求
                if (area > std::max(2000.0, (width * height) * 0.005)) {
                    cv::Rect boundRect = cv::boundingRect(contours[i]);
                    double aspectRatio = static_cast<double>(boundRect.width) / boundRect.height;
                    
                    // 更宽松的条件
                    if (aspectRatio > 0.2 && aspectRatio < 2.5 && 
                        boundRect.width > 40 && boundRect.height > 60) {
                        
                        cv::Mat coverRegion = roi(boundRect);
                        double quality = calculateRegionQuality(coverRegion);
                        
                        backupCandidates.emplace_back(boundRect.x, boundRect.y, 
                                                     boundRect.width, boundRect.height,
                                                     area, quality, static_cast<int>(i));
                        
                        qDebug() << QString("  策癱1候选 %1: 位置(%2,%3) 尺寸(%4x%5) 比例%6 质量%7")
                                    .arg(i).arg(boundRect.x).arg(boundRect.y)
                                    .arg(boundRect.width).arg(boundRect.height)
                                    .arg(aspectRatio, 0, 'f', 2).arg(quality, 0, 'f', 2);
                    }
                }
            }
            
            if (!backupCandidates.empty()) {
                qDebug() << QString("✓ 使用策癱1找到 %1 个候选区域").arg(backupCandidates.size());
                return backupCandidates;
            }
            
            // 策癱3: 选择最大轮廓
            qDebug() << "尝试降级处理策癱3: 选择最大轮廓...";
            if (!contours.empty()) {
                auto largestIt = std::max_element(contours.begin(), contours.end(),
                    [](const std::vector<cv::Point>& a, const std::vector<cv::Point>& b) {
                        return cv::contourArea(a) < cv::contourArea(b);
                    });
                
                double largestArea = cv::contourArea(*largestIt);
                
                if (largestArea > 1000) {
                    cv::Rect boundRect = cv::boundingRect(*largestIt);
                    if (boundRect.width > 30 && boundRect.height > 50) {
                        cv::Mat coverRegion = roi(boundRect);
                        double quality = calculateRegionQuality(coverRegion);
                        
                        candidates.emplace_back(boundRect.x, boundRect.y,
                                              boundRect.width, boundRect.height,
                                              largestArea, quality, 
                                              static_cast<int>(largestIt - contours.begin()));
                        
                        qDebug() << QString("✓ 使用最大轮廓: 位置(%1,%2) 尺寸(%3x%4) 面积%5")
                                    .arg(boundRect.x).arg(boundRect.y)
                                    .arg(boundRect.width).arg(boundRect.height)
                                    .arg(largestArea, 0, 'f', 0);
                    }
                }
            }
        }
        
    } catch (const std::exception& e) {
        qDebug() << "寻找候选区域时出错：" << e.what();
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
        
        // 多层边缘检测
        cv::Mat edges1, edges2, edges3;
        cv::Canny(blurred, edges1, 30, 80);
        cv::Canny(blurred, edges2, 50, 120);
        cv::Canny(blurred, edges3, 70, 150);
        
        // 合并多层边缘结果
        cv::bitwise_or(edges1, edges2, edges);
        cv::bitwise_or(edges, edges3, edges);
        
        // 形态学操作以连接断裂的边缘
        cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
        cv::morphologyEx(edges, edges, cv::MORPH_CLOSE, kernel);
        cv::morphologyEx(edges, edges, cv::MORPH_DILATE, kernel);
        
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
        
        // 尺寸加分（更大的区域通常更好）
        double sizeScore = std::min(2.0, (width * height) / 50000.0);
        
        // 转换为灰度
        cv::Mat gray;
        cv::cvtColor(region, gray, cv::COLOR_BGR2GRAY);
        
        // 计算纹理复杂度（标准差）
        cv::Scalar meanScalar, stdScalar;
        cv::meanStdDev(gray, meanScalar, stdScalar);
        double textureScore = std::min(2.0, stdScalar[0] / 30.0);
        
        // 计算边缘密度（更宽松的参数）
        cv::Mat edges;
        cv::Canny(gray, edges, 30, 100);  // 降低边缘检测阈值
        double edgeDensity = static_cast<double>(cv::countNonZero(edges)) / (height * width);
        double edgeScore = std::min(2.0, edgeDensity * 15.0);
        
        // 计算颜色分布多样性
        int histSize = 128;
        float range[] = {0, 256};
        const float* histRange = {range};
        cv::Mat hist;
        cv::calcHist(&gray, 1, 0, cv::Mat(), hist, 1, &histSize, &histRange);
        
        int colorDiversity = 0;
        for (int i = 0; i < histSize; i++) {
            if (hist.at<float>(i) > 0) {
                colorDiversity++;
            }
        }
        double diversityScore = std::min(2.0, colorDiversity / 30.0);
        
        // 计算亮度分布（避免过暗或过亮）
        double meanBrightness = meanScalar[0];
        double brightnessScore;
        if (meanBrightness > 30 && meanBrightness < 220) {  // 更宽松的亮度范围
            brightnessScore = 1.0;
        } else {
            brightnessScore = 0.3;
        }
        
        // 计算对比度
        double minVal, maxVal;
        cv::minMaxLoc(gray, &minVal, &maxVal);
        double contrast = maxVal - minVal;
        double contrastScore = std::min(1.0, contrast / 100.0);
        
        // 综合得分（权重优化，匹配Python版本）
        double qualityScore = (
            sizeScore * 0.2 +        // 尺寸权重
            textureScore * 0.25 +    // 纹理权重
            edgeScore * 0.2 +        // 边缘权重
            diversityScore * 0.15 +  // 多样性权重
            brightnessScore * 0.1 +  // 亮度权重
            contrastScore * 0.1      // 对比度权重
        );
        
        return std::max(0.1, std::min(qualityScore, 8.0));  // 确保有最小得分，避兀0得分
        
    } catch (const std::exception& e) {
        qDebug() << "质量评分计算错误：" << e.what();
        return 1.0;  // 返回默认得分而不是0
    }
}

cv::Mat CoverExtractor::extractFallbackByPosition(const cv::Mat& roi)
{
    try {
        int height = roi.rows;
        int width = roi.cols;
        
        // 尝试几个典型的封面位置
        std::vector<cv::Rect> positions = {
            cv::Rect(static_cast<int>(width * 0.02), static_cast<int>(height * 0.1), 
                    static_cast<int>(width * 0.35), static_cast<int>(height * 0.6)),
            cv::Rect(static_cast<int>(width * 0.05), static_cast<int>(height * 0.15), 
                    static_cast<int>(width * 0.3), static_cast<int>(height * 0.5)),
            cv::Rect(static_cast<int>(width * 0.01), static_cast<int>(height * 0.05), 
                    static_cast<int>(width * 0.4), static_cast<int>(height * 0.7))
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
            
            if (pos.width > 50 && pos.height > 80) {
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