#include "NetworkManager.h"
#include <QDebug>
#include <QStandardPaths>
#include <QDir>
#include "ConfigManager.h"

NetworkManager::NetworkManager(QObject* parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_timeoutInterval(30000) // 默认30秒超时
{
    // 设置默认用户代理
    m_globalUserAgent = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36";
}

NetworkManager::~NetworkManager()
{
    abortAllRequests();
}

NetworkManager& NetworkManager::getInstance()
{
    static NetworkManager instance;
    return instance;
}

void NetworkManager::sendGetRequest(const QString& url, NetworkResponseCallback callback, const QString& userAgent)
{
    QNetworkRequest request((QUrl(url)));
    
    // 设置用户代理
    QString effectiveUserAgent = userAgent.isEmpty() ? m_globalUserAgent : userAgent;
    request.setHeader(QNetworkRequest::UserAgentHeader, effectiveUserAgent);
    
    // 发送GET请求
    QNetworkReply* reply = m_networkManager->get(request);
    
    // 设置超时
    QTimer* timer = createTimeoutTimer(reply);
    
    // 连接完成信号
    connect(reply, &QNetworkReply::finished, this, [this, reply, callback, timer]() {
        timer->stop();
        timer->deleteLater();
        
        if (reply->error() == QNetworkReply::NoError) {
            // 读取响应数据
            QByteArray responseData = reply->readAll();
            callback(responseData, true);
        } else {
            qDebug() << "网络请求失败：" << reply->errorString();
            callback(QByteArray(), false);
        }
        
        // 清理资源
        reply->deleteLater();
    });
    
    // 连接错误信号
    connect(reply, &QNetworkReply::errorOccurred, this, [reply, callback, timer](QNetworkReply::NetworkError) {
        timer->stop();
        qDebug() << "网络错误：" << reply->errorString();
    });
}

void NetworkManager::downloadFile(const QString& url, 
                                  const QString& savePath,
                                  DownloadProgressCallback progressCallback,
                                  std::function<void(bool, const QString&)> finishedCallback,
                                  const QString& userAgent)
{
    // 确保目录存在
    QFileInfo fileInfo(savePath);
    QDir dir = fileInfo.absoluteDir();
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    // 创建文件
    QFile* file = new QFile(savePath);
    if (!file->open(QIODevice::WriteOnly)) {
        qDebug() << "无法创建文件：" << file->errorString();
        finishedCallback(false, "无法创建文件：" + file->errorString());
        delete file;
        return;
    }
    
    // 创建网络请求
    QNetworkRequest request((QUrl(url)));
    
    // 设置用户代理
    QString effectiveUserAgent = userAgent.isEmpty() ? m_globalUserAgent : userAgent;
    request.setHeader(QNetworkRequest::UserAgentHeader, effectiveUserAgent);
    
    // 开始下载
    QNetworkReply* reply = m_networkManager->get(request);
    m_currentDownloadReply = reply; // 保存当前下载的Reply对象
    
    // 设置超时
    QTimer* timer = createTimeoutTimer(reply);
    
    // 连接进度信号
    connect(reply, &QNetworkReply::downloadProgress, this, [progressCallback, timer](qint64 bytesReceived, qint64 bytesTotal) {
        timer->start(); // 重置定时器
        progressCallback(bytesReceived, bytesTotal);
    });
    
    // 连接读取数据信号
    connect(reply, &QNetworkReply::readyRead, this, [reply, file, timer]() {
        timer->start(); // 重置定时器
        file->write(reply->readAll());
    });
    
    // 连接完成信号
    connect(reply, &QNetworkReply::finished, this, [this, reply, file, finishedCallback, timer]() {
        timer->stop();
        timer->deleteLater();
        
        file->close();
        
        if (reply->error() == QNetworkReply::NoError) {
            finishedCallback(true, QString());
        } else {
            qDebug() << "下载失败：" << reply->errorString();
            file->remove(); // 删除不完整的文件
            finishedCallback(false, reply->errorString());
        }
        
        // 清理资源
        file->deleteLater();
        reply->deleteLater();
        
        // 清除当前下载的Reply对象
        if (m_currentDownloadReply == reply) {
            m_currentDownloadReply = nullptr;
        }
    });
    
    // 连接错误信号
    connect(reply, &QNetworkReply::errorOccurred, this, [file, timer](QNetworkReply::NetworkError) {
        timer->start(); // 重置定时器
    });
}

void NetworkManager::abortAllRequests()
{
    const auto replies = m_networkManager->findChildren<QNetworkReply*>();
    for (QNetworkReply* reply : replies) {
        reply->abort();
    }
}

void NetworkManager::setTimeoutInterval(int msec)
{
    m_timeoutInterval = msec;
}

void NetworkManager::setGlobalUserAgent(const QString& userAgent)
{
    m_globalUserAgent = userAgent;
}

QString NetworkManager::getGlobalUserAgent() const
{
    return m_globalUserAgent;
}

QTimer* NetworkManager::createTimeoutTimer(QNetworkReply* reply)
{
    QTimer* timer = new QTimer(this);
    timer->setSingleShot(true);
    timer->setInterval(m_timeoutInterval);
    
    connect(timer, &QTimer::timeout, this, &NetworkManager::onTimeoutTriggered);
    connect(timer, &QTimer::timeout, reply, [reply]() {
        if (reply && reply->isRunning()) {
            reply->abort();
        }
    });
    
    timer->start();
    return timer;
}

void NetworkManager::onTimeoutTriggered()
{
    qDebug() << "网络请求超时";
}

void NetworkManager::cancelDownload()
{
    if (m_currentDownloadReply && m_currentDownloadReply->isRunning()) {
        qDebug() << "取消当前下载";
        m_currentDownloadReply->abort();
    }
} 