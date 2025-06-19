#include "DownloadManager.h"
#include <QDebug>
#include <QFile>
#include <QFileInfo>

DownloadManager::DownloadManager(QObject* parent)
    : QObject(parent)
    , m_isDownloading(false)
{
}

void DownloadManager::downloadFile(const QString& url, 
                                  const QString& savePath,
                                  DLProgressCallback progressCallback,
                                  DLCompletedCallback completedCallback)
{    if (m_isDownloading) {
        qDebug() << "已有下载任务正在进行，请先取消当前下载";
        if (completedCallback) {
            completedCallback(false, "已有下载任务正在进行", savePath);
        }
        return;
    }
    
    // 清理URL
    QString cleanedUrl = cleanUrl(url);
      if (cleanedUrl.isEmpty()) {
        qDebug() << "下载URL为空";
        if (completedCallback) {
            completedCallback(false, "下载URL为空", savePath);
        }
        return;
    }
    
    m_isDownloading = true;
    
    // 使用NetworkManager下载文件
    NetworkManager::getInstance().downloadFile(
        cleanedUrl,
        savePath,
        [this, progressCallback](qint64 bytesReceived, qint64 bytesTotal) {
            if (bytesTotal > 0) {
                int progress = static_cast<int>((bytesReceived * 100) / bytesTotal);
                if (progressCallback) {
                    progressCallback(progress);
                }
            }
        },        [this, completedCallback, savePath](bool success, const QString& errorMsg) {
            m_isDownloading = false;
            
            if (success) {
                // 下载成功后，检查并修正文件扩展名
                QString correctedPath = correctFileExtension(savePath);
                
                // 如果文件路径发生了变化，说明扩展名被修正了
                bool pathChanged = (correctedPath != savePath);
                
                if (completedCallback) {
                    if (pathChanged) {
                        // 通知调用者文件名已更改
                        QFileInfo newFileInfo(correctedPath);
                        QString message = QString("文件格式已自动检测并修正为: %1").arg(newFileInfo.fileName());
                        completedCallback(true, message, correctedPath);
                    } else {
                        completedCallback(true, errorMsg, correctedPath);
                    }
                }
            } else {
                if (completedCallback) {
                    completedCallback(false, errorMsg, savePath); // 失败时返回原路径
                }
            }
        }
    );
}

void DownloadManager::downloadModifier(const ModifierInfo& modifier,
                                      const QString& version,
                                      const QString& savePath,
                                      std::function<void(bool, const QString&, const QString&, const ModifierInfo&, bool)> completedCallback,
                                      DLProgressCallback progressCallback)
{
    // 查找选择的版本链接
    QString url;
    for (const auto& ver : modifier.versions) {
        if (ver.first == version) {
            url = ver.second;
            break;
        }
    }
    
    if (url.isEmpty() && !modifier.versions.isEmpty()) {
        // 如果没有找到指定版本，使用第一个版本
        url = modifier.versions.first().second;
    }
      if (url.isEmpty()) {
        qDebug() << "未找到下载URL";
        if (completedCallback) {
            completedCallback(false, "未找到下载URL", savePath, modifier, false);
        }
        return;
    }
      // 检测文件格式
    bool isArchive = isArchiveFormat(url) || isArchiveFormat(savePath);
    
    if (isArchive) {
        qDebug() << "检测到压缩包格式文件：" << (isArchiveFormat(url) ? url : savePath);
        qDebug() << "文件扩展名：" << getFileExtension(isArchiveFormat(url) ? url : savePath);
    } else {
        qDebug() << "检测到普通文件格式";
    }
      // 下载文件
    downloadFile(
        url,
        savePath,
        progressCallback,
        [completedCallback, modifier, isArchive](bool success, const QString& errorMsg, const QString& actualPath) {
            if (completedCallback) {
                completedCallback(success, errorMsg, actualPath, modifier, isArchive);
            }
        }
    );
}

void DownloadManager::cancelDownload()
{
    if (m_isDownloading) {
        NetworkManager::getInstance().cancelDownload();
        m_isDownloading = false;
        qDebug() << "下载已取消";
    }
}

bool DownloadManager::isDownloading() const
{
    return m_isDownloading;
}

QString DownloadManager::cleanUrl(const QString& url) const
{
    QString cleanedUrl = url.trimmed();
    
    // 移除末尾的逗号
    while (cleanedUrl.endsWith(",")) {
        cleanedUrl.chop(1);
    }
    
    // 确保URL以http或https开头
    if (!cleanedUrl.startsWith("http://") && !cleanedUrl.startsWith("https://")) {
        qDebug() << "无效的URL格式：" << cleanedUrl;
        return QString();
    }
    
    return cleanedUrl;
}

bool DownloadManager::isArchiveFormat(const QString& filePath) const
{
    QString extension = getFileExtension(filePath);
    
    // 支持的压缩包格式
    QStringList archiveExtensions = {
        "zip", "rar", "7z", "tar", "gz", "bz2", "xz", "lzma",
        "tar.gz", "tar.bz2", "tar.xz", "tgz", "tbz2", "txz"
    };
    
    return archiveExtensions.contains(extension);
}

QString DownloadManager::getFileExtension(const QString& filePath) const
{
    QString path = filePath;
    
    // 如果是URL，先移除查询参数
    if (path.contains("?")) {
        path = path.split("?").first();
    }
    
    // 移除fragment部分
    if (path.contains("#")) {
        path = path.split("#").first();
    }
    
    // 获取文件名部分
    QString fileName = path.split("/").last();
    
    // 处理特殊的双扩展名
    if (fileName.contains(".tar.")) {
        if (fileName.endsWith(".gz")) return "tar.gz";
        if (fileName.endsWith(".bz2")) return "tar.bz2";
        if (fileName.endsWith(".xz")) return "tar.xz";
    }
    
    // 处理常见的缩写扩展名
    if (fileName.endsWith(".tgz")) return "tgz";
    if (fileName.endsWith(".tbz2")) return "tbz2";
    if (fileName.endsWith(".txz")) return "txz";
    
    // 获取普通扩展名
    if (!fileName.contains(".")) {
        return QString(); // 没有扩展名
    }
    
    return fileName.split(".").last().toLower();
}

QString DownloadManager::detectFileFormat(const QString& filePath) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "无法打开文件进行格式检测：" << filePath;
        return QString();
    }
    
    // 读取文件头部字节（魔数）
    QByteArray header = file.read(32); // 读取前32字节足够检测大多数格式
    file.close();
    
    if (header.isEmpty()) {
        return QString();
    }
    
    // 检测各种文件格式的魔数
    // ZIP 格式: 50 4B 03 04 或 50 4B 05 06 或 50 4B 07 08
    if (header.startsWith("\x50\x4B\x03\x04") || 
        header.startsWith("\x50\x4B\x05\x06") ||
        header.startsWith("\x50\x4B\x07\x08")) {
        return "zip";
    }
    
    // RAR 格式: 52 61 72 21 1A 07 00 (RAR!\x1A\x07\x00)
    if (header.startsWith("Rar!\x1A\x07\x00") || header.startsWith("Rar!\x1A\x07\x01")) {
        return "rar";
    }
    
    // 7Z 格式: 37 7A BC AF 27 1C
    if (header.startsWith("\x37\x7A\xBC\xAF\x27\x1C")) {
        return "7z";
    }
    
    // PE 可执行文件 (EXE): MZ (4D 5A)
    if (header.startsWith("MZ")) {
        return "exe";
    }
    
    // GZIP 格式: 1F 8B
    if (header.startsWith("\x1F\x8B")) {
        return "gz";
    }
    
    // BZIP2 格式: 42 5A 68 (BZh)
    if (header.startsWith("BZh")) {
        return "bz2";
    }
    
    // TAR 格式检测（TAR文件在offset 257处有"ustar"标识）
    file.open(QIODevice::ReadOnly);
    if (file.size() > 262) {
        file.seek(257);
        QByteArray tarSignature = file.read(5);
        if (tarSignature == "ustar") {
            file.close();
            return "tar";
        }
    }
    file.close();
    
    qDebug() << "无法识别文件格式，文件头：" << header.left(16).toHex();
    return QString();
}

QString DownloadManager::correctFileExtension(const QString& filePath) const
{
    QString detectedFormat = detectFileFormat(filePath);
    if (detectedFormat.isEmpty()) {
        qDebug() << "无法检测文件格式，保持原文件名：" << filePath;
        return filePath;
    }
    
    QString currentExtension = getFileExtension(filePath);
    
    // 如果检测到的格式与当前扩展名匹配，不需要修正
    if (currentExtension == detectedFormat) {
        qDebug() << "文件格式与扩展名匹配：" << detectedFormat;
        return filePath;
    }
    
    // 需要修正扩展名
    QFileInfo fileInfo(filePath);
    QString baseName = fileInfo.completeBaseName();
    QString newPath = fileInfo.absolutePath() + "/" + baseName + "." + detectedFormat;
    
    qDebug() << "检测到文件格式不匹配：";
    qDebug() << "  当前扩展名：" << currentExtension;
    qDebug() << "  检测到的格式：" << detectedFormat;
    qDebug() << "  原文件名：" << filePath;
    qDebug() << "  新文件名：" << newPath;
    
    // 重命名文件
    QFile file(filePath);
    if (file.rename(newPath)) {
        qDebug() << "文件重命名成功：" << newPath;
        return newPath;
    } else {
        qDebug() << "文件重命名失败，保持原文件名：" << filePath;
        return filePath;
    }
} 