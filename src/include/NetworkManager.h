#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QFile>
#include <QTimer>
#include <QString>
#include <QUrl>
#include <QHash>
#include <functional>
#include "ModifierParser.h"

// Network response callback function type definition
using NetworkResponseCallback = std::function<void(const QByteArray&, bool)>;
using DownloadProgressCallback = std::function<void(qint64, qint64)>;
using DownloadFinishedCallback = std::function<void(bool, const QString&, int)>;
using TestGetRequestHandler =
    std::function<bool(const QString&, const QString&, NetworkResponseCallback)>;
using TestDownloadRequestHandler =
    std::function<bool(const QString&,
                       const QString&,
                       const QString&,
                       qint64,
                       bool,
                       DownloadProgressCallback,
                       DownloadFinishedCallback)>;

class NetworkManager : public QObject
{
    Q_OBJECT

public:
    static NetworkManager& getInstance();

    // Send GET request
    void sendGetRequest(const QString& url, 
                        NetworkResponseCallback callback, 
                        const QString& userAgent = QString());
    void sendGetRequest(const QString& url,
                        QObject* context,
                        NetworkResponseCallback callback,
                        const QString& userAgent = QString());

    // Download file
    void downloadFile(const QString& url, 
                      const QString& savePath,
                      DownloadProgressCallback progressCallback,
                      std::function<void(bool, const QString&)> finishedCallback,
                      const QString& userAgent = QString(),
                      qint64 resumeFrom = 0,
                      bool keepPartialOnAbort = false);
    void downloadFile(const QString& url,
                      const QString& savePath,
                      QObject* context,
                      DownloadProgressCallback progressCallback,
                      std::function<void(bool, const QString&)> finishedCallback,
                      const QString& userAgent = QString(),
                      qint64 resumeFrom = 0,
                      bool keepPartialOnAbort = false);

    // Download file with HTTP metadata callback.
    void downloadFileWithStatus(const QString& url,
                                const QString& savePath,
                                DownloadProgressCallback progressCallback,
                                DownloadFinishedCallback finishedCallback,
                                const QString& userAgent = QString(),
                                qint64 resumeFrom = 0,
                                bool keepPartialOnAbort = false);
    void downloadFileWithStatus(const QString& url,
                                const QString& savePath,
                                QObject* context,
                                DownloadProgressCallback progressCallback,
                                DownloadFinishedCallback finishedCallback,
                                const QString& userAgent = QString(),
                                qint64 resumeFrom = 0,
                                bool keepPartialOnAbort = false);

    // Abort all requests
    void abortAllRequests();
    
    // Cancel the download writing to savePath. Downloads are tracked per
    // destination path, so cancelling one transfer never aborts another.
    void cancelDownload(const QString& savePath);

    // Set global timeout interval (milliseconds)
    void setTimeoutInterval(int msec);

    // Set global user agent
    void setGlobalUserAgent(const QString& userAgent);

    // Get global user agent
    QString getGlobalUserAgent() const;

    // Same-origin Referer for url ("https://host/"), or empty when url carries
    // no scheme or host. Some hosts serve downloads to same-origin referrers only.
    static QString defaultRefererForUrl(const QString& url);

    // Test hooks
    void setGetRequestHandlerForTesting(TestGetRequestHandler handler);
    void setDownloadRequestHandlerForTesting(TestDownloadRequestHandler handler);
    void resetTestHooks();

private:
    NetworkManager(QObject* parent = nullptr);
    ~NetworkManager();

    // Disable copy and assignment
    NetworkManager(const NetworkManager&) = delete;
    NetworkManager& operator=(const NetworkManager&) = delete;

    // Create timeout timer for request
    QTimer* createTimeoutTimer(QNetworkReply* reply);

    // Drop savePath from the active-download table, but only when it still
    // maps to this reply (a later download to the same path wins).
    void unregisterDownload(const QString& savePath, QNetworkReply* reply);

private:
    QNetworkAccessManager* m_networkManager;
    int m_timeoutInterval;
    QString m_globalUserAgent;
    QHash<QString, QNetworkReply*> m_activeDownloads;
    TestGetRequestHandler m_testGetRequestHandler;
    TestDownloadRequestHandler m_testDownloadRequestHandler;

private slots:
    void onTimeoutTriggered();
}; 
