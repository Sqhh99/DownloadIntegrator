#pragma once

#include <QObject>
#include <QString>
#include <functional>
#include "ModifierParser.h"
#include "NetworkManager.h"
#include "FileSystem.h"

// 回调函数类型 - 使用不同名称避免冲突
using DLProgressCallback = std::function<void(int)>; // 下载进度回调（百分比）
using DLCompletedCallback = std::function<void(bool, const QString&)>; // 下载完成回调

/**
 * @brief 下载管理器类，专门处理文件下载相关功能
 */
class DownloadManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 获取单例实例
     * @return DownloadManager对象的引用
     */
    static DownloadManager& getInstance() {
        static DownloadManager instance;
        return instance;
    }
    
    /**
     * @brief 下载文件
     * @param url 下载URL
     * @param savePath 保存路径
     * @param progressCallback 进度回调
     * @param completedCallback 完成回调
     */
    void downloadFile(const QString& url, 
                     const QString& savePath,
                     DLProgressCallback progressCallback,
                     DLCompletedCallback completedCallback);
    
    /**
     * @brief 下载修改器
     * @param modifier 修改器信息
     * @param version 选择的版本
     * @param savePath 保存路径
     * @param completedCallback 完成回调
     * @param progressCallback 进度回调
     */
    void downloadModifier(const ModifierInfo& modifier,
                         const QString& version,
                         const QString& savePath,
                         std::function<void(bool, const QString&, const ModifierInfo&)> completedCallback,
                         DLProgressCallback progressCallback);
    
    /**
     * @brief 取消当前下载
     */
    void cancelDownload();
    
    /**
     * @brief 获取下载状态
     * @return 是否有下载正在进行
     */
    bool isDownloading() const;
    
    /**
     * @brief 清理无效URL，移除末尾的逗号等
     * @param url 原始URL
     * @return 清理后的URL
     */
    QString cleanUrl(const QString& url) const;

private:
    // 私有构造函数
    DownloadManager(QObject* parent = nullptr);
    ~DownloadManager() = default;
    
    // 禁止拷贝和赋值
    DownloadManager(const DownloadManager&) = delete;
    DownloadManager& operator=(const DownloadManager&) = delete;
    
private:
    bool m_isDownloading;
}; 