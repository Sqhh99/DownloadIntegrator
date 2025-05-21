#include "DownloadManager.h"
#include <QDebug>

DownloadManager::DownloadManager(QObject* parent)
    : QObject(parent)
    , m_isDownloading(false)
{
}

void DownloadManager::downloadFile(const QString& url, 
                                  const QString& savePath,
                                  DLProgressCallback progressCallback,
                                  DLCompletedCallback completedCallback)
{
    if (m_isDownloading) {
        qDebug() << "已有下载任务正在进行，请先取消当前下载";
        if (completedCallback) {
            completedCallback(false, "已有下载任务正在进行");
        }
        return;
    }
    
    // 清理URL
    QString cleanedUrl = cleanUrl(url);
    
    if (cleanedUrl.isEmpty()) {
        qDebug() << "下载URL为空";
        if (completedCallback) {
            completedCallback(false, "下载URL为空");
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
        },
        [this, completedCallback](bool success, const QString& errorMsg) {
            m_isDownloading = false;
            
            if (completedCallback) {
                completedCallback(success, errorMsg);
            }
        }
    );
}

void DownloadManager::downloadModifier(const ModifierInfo& modifier,
                                      const QString& version,
                                      const QString& savePath,
                                      std::function<void(bool, const QString&, const ModifierInfo&)> completedCallback,
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
            completedCallback(false, "未找到下载URL", modifier);
        }
        return;
    }
    
    // 下载文件
    downloadFile(
        url,
        savePath,
        progressCallback,
        [completedCallback, modifier](bool success, const QString& errorMsg) {
            if (completedCallback) {
                completedCallback(success, errorMsg, modifier);
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