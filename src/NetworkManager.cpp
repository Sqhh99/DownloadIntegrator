#include "NetworkManager.h"
#include <QDebug>
#include <QStandardPaths>
#include <QDir>
#include <QPointer>
#include <utility>
#include "ConfigManager.h"
#include "Logger.h"

NetworkManager::NetworkManager(QObject* parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_timeoutInterval(30000) // Default 30 second timeout
{
    // Set default user agent
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

void NetworkManager::setGetRequestHandlerForTesting(TestGetRequestHandler handler)
{
    m_testGetRequestHandler = std::move(handler);
}

void NetworkManager::setDownloadRequestHandlerForTesting(TestDownloadRequestHandler handler)
{
    m_testDownloadRequestHandler = std::move(handler);
}

void NetworkManager::resetTestHooks()
{
    m_testGetRequestHandler = nullptr;
    m_testDownloadRequestHandler = nullptr;
}

void NetworkManager::sendGetRequest(const QString& url, NetworkResponseCallback callback, const QString& userAgent)
{
    sendGetRequest(url, nullptr, callback, userAgent);
}

void NetworkManager::sendGetRequest(const QString& url,
                                    QObject* context,
                                    NetworkResponseCallback callback,
                                    const QString& userAgent)
{
    QNetworkRequest request((QUrl(url)));

    // Set user agent
    QString effectiveUserAgent = userAgent.isEmpty() ? m_globalUserAgent : userAgent;
    QPointer<QObject> contextGuard(context);
    const NetworkResponseCallback safeCallback = [callback, context, contextGuard](const QByteArray& data, bool success) {
        if (callback && (!context || contextGuard)) {
            callback(data, success);
        }
    };

    if (m_testGetRequestHandler && m_testGetRequestHandler(url, effectiveUserAgent, safeCallback)) {
        return;
    }

    request.setHeader(QNetworkRequest::UserAgentHeader, effectiveUserAgent);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setAttribute(QNetworkRequest::Http2AllowedAttribute, false);

    // Send GET request
    QNetworkReply* reply = m_networkManager->get(request);
    
    // Set timeout
    QTimer* timer = createTimeoutTimer(reply);
    
    // Connect finished signal
    connect(reply, &QNetworkReply::finished, this, [this, reply, callback, timer, context, contextGuard]() {
        timer->stop();
        timer->deleteLater();
        
        if (reply->error() == QNetworkReply::NoError) {
            // Read response data
            QByteArray responseData = reply->readAll();
            if (callback && (!context || contextGuard)) {
                callback(responseData, true);
            }
        } else {
            LOG_DEBUG() << "Network request failed:" << reply->errorString();
            if (callback && (!context || contextGuard)) {
                callback(QByteArray(), false);
            }
        }
        
        // Clean up resources
        reply->deleteLater();
    });
    
    // Connect error signal
    connect(reply, &QNetworkReply::errorOccurred, this, [reply, callback, timer](QNetworkReply::NetworkError) {
        timer->stop();
        LOG_DEBUG() << "Network error:" << reply->errorString();
    });
}

void NetworkManager::downloadFile(const QString& url, 
                                  const QString& savePath,
                                  DownloadProgressCallback progressCallback,
                                  std::function<void(bool, const QString&)> finishedCallback,
                                  const QString& userAgent,
                                  qint64 resumeFrom,
                                  bool keepPartialOnAbort)
{
    downloadFile(url,
                 savePath,
                 nullptr,
                 progressCallback,
                 finishedCallback,
                 userAgent,
                 resumeFrom,
                 keepPartialOnAbort);
}

void NetworkManager::downloadFile(const QString& url,
                                  const QString& savePath,
                                  QObject* context,
                                  DownloadProgressCallback progressCallback,
                                  std::function<void(bool, const QString&)> finishedCallback,
                                  const QString& userAgent,
                                  qint64 resumeFrom,
                                  bool keepPartialOnAbort)
{
    downloadFileWithStatus(
        url,
        savePath,
        context,
        progressCallback,
        [finishedCallback](bool success, const QString& errorMsg, int) {
            if (finishedCallback) {
                finishedCallback(success, errorMsg);
            }
        },
        userAgent,
        resumeFrom,
        keepPartialOnAbort);
}

void NetworkManager::downloadFileWithStatus(const QString& url,
                                            const QString& savePath,
                                            DownloadProgressCallback progressCallback,
                                            DownloadFinishedCallback finishedCallback,
                                            const QString& userAgent,
                                            qint64 resumeFrom,
                                            bool keepPartialOnAbort)
{
    downloadFileWithStatus(url,
                           savePath,
                           nullptr,
                           progressCallback,
                           finishedCallback,
                           userAgent,
                           resumeFrom,
                           keepPartialOnAbort);
}

void NetworkManager::downloadFileWithStatus(const QString& url,
                                            const QString& savePath,
                                            QObject* context,
                                            DownloadProgressCallback progressCallback,
                                            DownloadFinishedCallback finishedCallback,
                                            const QString& userAgent,
                                            qint64 resumeFrom,
                                            bool keepPartialOnAbort)
{
    LOG_DEBUG() << "NetworkManager: Starting download from:" << url;
    LOG_DEBUG() << "NetworkManager: Save path:" << savePath;
    QPointer<QObject> contextGuard(context);
    const DownloadProgressCallback safeProgressCallback =
        [progressCallback, context, contextGuard](qint64 bytesReceived, qint64 bytesTotal) {
            if (progressCallback && (!context || contextGuard)) {
                progressCallback(bytesReceived, bytesTotal);
            }
        };
    const DownloadFinishedCallback safeFinishedCallback =
        [finishedCallback, context, contextGuard](bool success, const QString& errorMessage, int statusCode) {
            if (finishedCallback && (!context || contextGuard)) {
                finishedCallback(success, errorMessage, statusCode);
            }
        };

    QString effectiveUserAgent = userAgent.isEmpty() ? m_globalUserAgent : userAgent;
    if (m_testDownloadRequestHandler &&
        m_testDownloadRequestHandler(url,
                                     savePath,
                                     effectiveUserAgent,
                                     resumeFrom,
                                     keepPartialOnAbort,
                                     safeProgressCallback,
                                     safeFinishedCallback)) {
        return;
    }
    
    // Ensure directory exists
    QFileInfo fileInfo(savePath);
    QDir dir = fileInfo.absoluteDir();
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    // Create file
    QFile* file = new QFile(savePath);
    const QIODeviceBase::OpenMode openMode = (resumeFrom > 0)
        ? (QIODevice::WriteOnly | QIODevice::Append)
        : (QIODevice::WriteOnly | QIODevice::Truncate);
    if (!file->open(openMode)) {
        LOG_DEBUG() << "NetworkManager: Cannot create file:" << file->errorString();
        if (safeFinishedCallback) {
            safeFinishedCallback(false, "Cannot create file: " + file->errorString(), 0);
        }
        delete file;
        return;
    }
    
    // Create network request
    QNetworkRequest request((QUrl(url)));
    
    // Set user agent
    request.setHeader(QNetworkRequest::UserAgentHeader, effectiveUserAgent);
    request.setAttribute(QNetworkRequest::Http2AllowedAttribute, false);
    
    // Enable automatic redirect handling
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, 
                        QNetworkRequest::NoLessSafeRedirectPolicy);
    
    // flingtrainer.com hands trainer payloads out only to same-origin referrers:
    // the final /wp-content/uploads/trainer-files/... hop answers 403 without this.
    // Qt copies request headers onto redirect requests, so setting it once here
    // covers every hop of the chain.
    const QString referer = defaultRefererForUrl(url);
    if (!referer.isEmpty()) {
        request.setRawHeader("Referer", referer.toUtf8());
    }
    
    // Resume partial download with HTTP Range when possible.
    if (resumeFrom > 0) {
        request.setRawHeader("Range", QString("bytes=%1-").arg(resumeFrom).toUtf8());
    }
    
    // Start download
    QNetworkReply* reply = m_networkManager->get(request);
    m_activeDownloads.insert(savePath, reply); // Track this transfer by destination
    
    // Set timeout
    QTimer* timer = createTimeoutTimer(reply);
    
    // Track bytes written
    qint64* bytesWritten = new qint64(0);
    qint64* effectiveResumeFrom = new qint64(resumeFrom);
    bool* responseModeChecked = new bool(false);
    bool* writeFailed = new bool(false);
    
    auto normalizeResponseMode = [reply, file, resumeFrom, effectiveResumeFrom, responseModeChecked, bytesWritten]() {
        if (*responseModeChecked || resumeFrom <= 0) {
            return;
        }
        
        int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (statusCode == 200) {
            // Server ignored Range; restart from zero with current response body.
            file->resize(0);
            file->seek(0);
            *effectiveResumeFrom = 0;
            *bytesWritten = 0;
            LOG_DEBUG() << "NetworkManager: Range not supported, restarting current response from byte 0";
        } else if (statusCode == 206) {
            *effectiveResumeFrom = resumeFrom;
            LOG_DEBUG() << "NetworkManager: Resuming from byte offset:" << resumeFrom;
        }
        
        *responseModeChecked = true;
    };
    
    connect(reply, &QNetworkReply::metaDataChanged, this, normalizeResponseMode);
    
    // Connect progress signal
    connect(reply, &QNetworkReply::downloadProgress, this, [safeProgressCallback, timer, effectiveResumeFrom](qint64 bytesReceived, qint64 bytesTotal) {
        timer->start(); // Reset timer
        const qint64 logicalReceived = *effectiveResumeFrom + bytesReceived;
        const qint64 logicalTotal = (bytesTotal > 0) ? (*effectiveResumeFrom + bytesTotal) : 0;
        if (safeProgressCallback) {
            safeProgressCallback(logicalReceived, logicalTotal);
        }
    });
    
    // Connect data ready signal
    connect(reply, &QNetworkReply::readyRead, this, [reply, file, timer, bytesWritten, writeFailed, normalizeResponseMode]() {
        timer->start(); // Reset timer
        normalizeResponseMode();
        QByteArray data = reply->readAll();
        // Count bytes persisted, not bytes received: a full disk or a write
        // error must not be reported as a completed download.
        const qint64 written = file->write(data);
        if (written != data.size()) {
            *writeFailed = true;
            LOG_WARN() << "NetworkManager: Short write to" << file->fileName()
                       << "- wrote" << written << "of" << data.size()
                       << "bytes:" << file->errorString();
        }
        if (written > 0) {
            *bytesWritten += written;
        }
    });
    
    // Connect finished signal
    connect(reply, &QNetworkReply::finished, this, [this, reply, file, timer, bytesWritten, effectiveResumeFrom, responseModeChecked, writeFailed, normalizeResponseMode, url, savePath, keepPartialOnAbort, safeFinishedCallback]() {
        normalizeResponseMode();
        timer->stop();
        timer->deleteLater();
        
        // Inspect the write state before close(); QFile clears its error there.
        if (!file->flush() || file->error() != QFileDevice::NoError) {
            *writeFailed = true;
        }
        const QString writeErrorString = file->errorString();
        file->close();
        
        // Get HTTP status code
        int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        QUrl redirectUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
        
        LOG_DEBUG() << "NetworkManager: Download finished for:" << url;
        LOG_DEBUG() << "NetworkManager: HTTP status code:" << httpStatus;
        LOG_DEBUG() << "NetworkManager: Bytes written:" << *bytesWritten;
        LOG_DEBUG() << "NetworkManager: Network error:" << reply->error() << reply->errorString();
        
        if (!redirectUrl.isEmpty()) {
            LOG_DEBUG() << "NetworkManager: Redirect URL detected:" << redirectUrl;
        }
        
        if (reply->error() == QNetworkReply::NoError) {
            // Check if we actually received data
            QFileInfo downloadedFile(savePath);
            qint64 fileSize = downloadedFile.size();
            
            LOG_DEBUG() << "NetworkManager: Final file size:" << fileSize << "bytes";
            
            if (*writeFailed) {
                // The transfer completed but the bytes did not reach the disk
                // (full volume, permission loss, I/O error). Reporting success
                // here would rename a truncated archive into the library.
                LOG_WARN() << "NetworkManager: Download data could not be written to"
                           << savePath << ":" << writeErrorString;
                if (!keepPartialOnAbort) {
                    file->remove();
                }
                delete bytesWritten;
                delete effectiveResumeFrom;
                delete responseModeChecked;
                delete writeFailed;
                if (safeFinishedCallback) {
                    safeFinishedCallback(false,
                                         "Failed to write downloaded data to disk: " + writeErrorString,
                                         httpStatus);
                }
                file->deleteLater();
                reply->deleteLater();
                unregisterDownload(savePath, reply);
                return;
            }
            
            if (fileSize == 0 || *bytesWritten == 0) {
                LOG_DEBUG() << "NetworkManager: WARNING - Downloaded file is empty!";
                LOG_DEBUG() << "NetworkManager: Response headers:";
                for (const auto& header : reply->rawHeaderPairs()) {
                    LOG_DEBUG() << "  " << header.first << ":" << header.second;
                }
                
                // Check Content-Type to see if it's an error page
                QString contentType = reply->header(QNetworkRequest::ContentTypeHeader).toString();
                LOG_DEBUG() << "NetworkManager: Content-Type:" << contentType;
                
                // If we got HTML instead of a file, it's likely an error page
                if (contentType.contains("text/html", Qt::CaseInsensitive)) {
                    file->remove();
                    delete bytesWritten;
                    delete effectiveResumeFrom;
                    delete responseModeChecked;
                    delete writeFailed;
                    if (safeFinishedCallback) {
                        safeFinishedCallback(false, "Server returned HTML page instead of file - download link may be invalid", httpStatus);
                    }
                    reply->deleteLater();
                    file->deleteLater();
                    unregisterDownload(savePath, reply);
                    return;
                }
                
                file->remove();
                delete bytesWritten;
                delete effectiveResumeFrom;
                delete responseModeChecked;
                delete writeFailed;
                if (safeFinishedCallback) {
                    safeFinishedCallback(false, "Downloaded file is empty - server may have returned no content", httpStatus);
                }
            } else {
                delete bytesWritten;
                delete effectiveResumeFrom;
                delete responseModeChecked;
                delete writeFailed;
                if (safeFinishedCallback) {
                    safeFinishedCallback(true, QString(), httpStatus);
                }
            }
        } else {
            LOG_DEBUG() << "NetworkManager: Download failed:" << reply->errorString();
            if (httpStatus == 416) {
                QFileInfo existingFile(savePath);
                if (existingFile.exists() && existingFile.size() > 0) {
                    delete bytesWritten;
                    delete effectiveResumeFrom;
                    delete responseModeChecked;
                    delete writeFailed;
                    if (safeFinishedCallback) {
                        safeFinishedCallback(true, QString(), httpStatus);
                    }
                    file->deleteLater();
                    reply->deleteLater();
                    unregisterDownload(savePath, reply);
                    return;
                }
            }
            
            const bool isCanceled = (reply->error() == QNetworkReply::OperationCanceledError);
            if (!(keepPartialOnAbort && isCanceled)) {
                file->remove(); // Delete incomplete file
            }
            delete bytesWritten;
            delete effectiveResumeFrom;
            delete responseModeChecked;
            delete writeFailed;
            if (safeFinishedCallback) {
                safeFinishedCallback(false, reply->errorString(), httpStatus);
            }
        }
        
        // Clean up resources
        file->deleteLater();
        reply->deleteLater();
        
        unregisterDownload(savePath, reply);
    });
    
    // Connect error signal
    connect(reply, &QNetworkReply::errorOccurred, this, [reply, timer](QNetworkReply::NetworkError error) {
        LOG_DEBUG() << "NetworkManager: Error occurred during download:" << error << reply->errorString();
        timer->start(); // Reset timer
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

QString NetworkManager::defaultRefererForUrl(const QString& url)
{
    const QUrl parsed(url);
    if (parsed.scheme().isEmpty() || parsed.host().isEmpty()) {
        return QString();
    }

    // Scheme, host and any non-default port only - never the path, which can
    // carry a download token we have no reason to leak to a redirect target.
    QUrl origin;
    origin.setScheme(parsed.scheme());
    origin.setHost(parsed.host());
    origin.setPort(parsed.port());
    origin.setPath(QStringLiteral("/"));
    return origin.toString();
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
    LOG_DEBUG() << "Network request timeout";
}

void NetworkManager::unregisterDownload(const QString& savePath, QNetworkReply* reply)
{
    const auto it = m_activeDownloads.constFind(savePath);
    if (it != m_activeDownloads.cend() && it.value() == reply) {
        m_activeDownloads.erase(it);
    }
}

void NetworkManager::cancelDownload(const QString& savePath)
{
    QNetworkReply* reply = m_activeDownloads.value(savePath);
    if (reply && reply->isRunning()) {
        LOG_DEBUG() << "Cancelling download:" << savePath;
        reply->abort();
    }
}
