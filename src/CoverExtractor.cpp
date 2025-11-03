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
        
        // 优化1：如果图片太大，先缩小处理，提升速度（不影响检测效果）
        cv::Mat processImage = image;
        float scale = 1.0f;
        const int MAX_PROCESS_WIDTH = 800;
        if (width > MAX_PROCESS_WIDTH) {
            scale = static_cast<float>(MAX_PROCESS_WIDTH) / width;
            cv::resize(image, processImage, cv::Size(), scale, scale, cv::INTER_LINEAR);
            qDebug() << QString("图片缩放: %1 -> 速度提升约 %2 倍").arg(scale).arg(1.0/scale);
        }
        
        // 专注于左上角区域（游戏封面通常在此位置）
        int roiWidth = static_cast<int>(processImage.cols * 0.4);
        int roiHeight = static_cast<int>(processImage.rows * 0.8);
        cv::Rect roiRect(0, 0, roiWidth, roiHeight);
        cv::Mat roi = processImage(roiRect);
        
        qDebug() << QString("分析区域尺寸: %1 x %2").arg(roiWidth).arg(roiHeight);
        
        // 查找封面候选区域
        std::vector<CoverCandidate> candidates = findCoverCandidates(roi);
        
        if (!candidates.empty()) {
            // 优化2：只取最好的候选，不需要排序所有
            auto best_it = std::max_element(candidates.begin(), candidates.end(),
                [](const CoverCandidate& a, const CoverCandidate& b) {
                    return (a.area * a.quality) < (b.area * b.quality);
                });
            
            const CoverCandidate& best = *best_it;
            qDebug() << QString("选择最佳候选区域: 位置(%1,%2) 尺寸(%3x%4)")
                        .arg(best.x).arg(best.y).arg(best.w).arg(best.h);
            
            // 提取封面区域（如果缩放了，需要映射回原图）
            cv::Rect coverRect;
            cv::Mat cover;
            if (scale < 1.0f) {
                // 映射回原图坐标
                coverRect = cv::Rect(
                    static_cast<int>(best.x / scale),
                    static_cast<int>(best.y / scale),
                    static_cast<int>(best.w / scale),
                    static_cast<int>(best.h / scale)
                );
                // 确保不超出原图边界
                coverRect.x = std::max(0, std::min(coverRect.x, width - 1));
                coverRect.y = std::max(0, std::min(coverRect.y, height - 1));
                coverRect.width = std::min(coverRect.width, width - coverRect.x);
                coverRect.height = std::min(coverRect.height, height - coverRect.y);
                
                // 从原图中提取（保持原始分辨率）
                int origRoiWidth = static_cast<int>(width * 0.4);
                int origRoiHeight = static_cast<int>(height * 0.8);
                cv::Rect origRoiRect(0, 0, origRoiWidth, origRoiHeight);
                cv::Mat origRoi = image(origRoiRect);
                cover = origRoi(coverRect);
            } else {
                coverRect = cv::Rect(best.x, best.y, best.w, best.h);
                cover = roi(coverRect);
            }
            
            // 优化3：边框优化是可选的，只对大图进行
            if (cover.cols > 150 || cover.rows > 200) {
                cv::Mat optimizedCover = removeCoverBorders(cover);
                return optimizedCover.empty() ? cover : optimizedCover;
            }
            
            return cover;
        } else {
            qDebug() << "未找到符合条件的封面区域，尝试备用方案";
            // 备用方案也使用缩放后的图
            cv::Mat fallback = extractFallbackByPosition(roi);
            if (!fallback.empty() && scale < 1.0f) {
                // 从原图中提取对应区域
                int origRoiWidth = static_cast<int>(width * 0.4);
                int origRoiHeight = static_cast<int>(height * 0.8);
                cv::Rect origRoiRect(0, 0, origRoiWidth, origRoiHeight);
                cv::Mat origRoi = image(origRoiRect);
                
                cv::Rect pos(static_cast<int>(origRoi.cols * 0.02), static_cast<int>(origRoi.rows * 0.1), 
                            static_cast<int>(origRoi.cols * 0.35), static_cast<int>(origRoi.rows * 0.6));
                pos.x = std::max(0, std::min(pos.x, origRoi.cols - 1));
                pos.y = std::max(0, std::min(pos.y, origRoi.rows - 1));
                pos.width = std::min(pos.width, origRoi.cols - pos.x);
                pos.height = std::min(pos.height, origRoi.rows - pos.y);
                
                return origRoi(pos).clone();
            }
            return fallback;
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
        
        // 设置最小面积阈值
        double minArea = std::max(5000.0, (width * height) * 0.015);
        
        // 优化4：提前筛选，只处理符合条件的轮廓
        for (size_t i = 0; i < contours.size(); ++i) {
            double area = cv::contourArea(contours[i]);
            
            if (area > minArea) {
                cv::Rect boundRect = cv::boundingRect(contours[i]);
                double aspectRatio = static_cast<double>(boundRect.width) / boundRect.height;
                
                // 快速筛选：长宽比和尺寸
                if (aspectRatio > 0.5 && aspectRatio < 1.2 &&
                    boundRect.width > 80 && boundRect.height > 120 &&
                    boundRect.x > 3 && boundRect.y > 3) {
                    
                    // 优化5：延迟质量评分，只对通过初筛的进行
                    cv::Mat coverRegion = roi(boundRect);
                    double quality = calculateRegionQuality(coverRegion);
                    
                    candidates.emplace_back(boundRect.x, boundRect.y, 
                                          boundRect.width, boundRect.height,
                                          area, quality, static_cast<int>(i));
                    
                    // 优化6：如果找到了非常好的候选（面积大且质量高），提前退出
                    if (area > minArea * 3 && quality > 0.8) {
                        qDebug() << QString("✓ 找到优质候选区域，提前结束搜索");
                        break;
                    }
                }
            }
        }
        
        // 降级策略
        if (candidates.empty() && !contours.empty()) {
            qDebug() << "使用降级策略：选择最大轮廓";
            
            auto largestIt = std::max_element(contours.begin(), contours.end(),
                [](const std::vector<cv::Point>& a, const std::vector<cv::Point>& b) {
                    return cv::contourArea(a) < cv::contourArea(b);
                });
            
            double largestArea = cv::contourArea(*largestIt);
            
            if (largestArea > 2000) {
                cv::Rect boundRect = cv::boundingRect(*largestIt);
                if (boundRect.width > 40 && boundRect.height > 60) {
                    cv::Mat coverRegion = roi(boundRect);
                    double quality = calculateRegionQuality(coverRegion);
                    
                    candidates.emplace_back(boundRect.x, boundRect.y,
                                          boundRect.width, boundRect.height,
                                          largestArea, quality, 
                                          static_cast<int>(largestIt - contours.begin()));
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
        
        // 尺寸加分
        double sizeScore = std::min(2.0, (width * height) / 50000.0);
        
        // 转换为灰度
        cv::Mat gray;
        cv::cvtColor(region, gray, cv::COLOR_BGR2GRAY);
        
        // 计算纹理复杂度（标准差）
        cv::Scalar meanScalar, stdScalar;
        cv::meanStdDev(gray, meanScalar, stdScalar);
        double textureScore = std::min(2.0, stdScalar[0] / 30.0);
        
        // 优化：去掉耗时的边缘密度计算
        // 计算亮度分布
        double meanBrightness = meanScalar[0];
        double brightnessScore = (meanBrightness > 30 && meanBrightness < 220) ? 1.0 : 0.3;
        
        // 优化：去掉耗时的颜色分布多样性计算和对比度计算
        // 简化的综合得分
        double qualityScore = (
            sizeScore * 0.5 +        // 增加尺寸权重
            textureScore * 0.4 +     // 纹理权重
            brightnessScore * 0.1    // 亮度权重
        );
        
        return std::max(0.1, std::min(qualityScore, 8.0));
        
    } catch (const std::exception& e) {
        qDebug() << "质量评分计算错误：" << e.what();
        return 1.0;
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