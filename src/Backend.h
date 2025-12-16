#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QVariantList>
#include "ModifierListModel.h"
#include "DownloadedModifierModel.h"
#include "ModifierParser.h"
#include "SearchManager.h"
#include "ModifierManager.h"
#include "ConfigManager.h"
#include "ThemeManager.h"
#include "LanguageManager.h"
#include "SettingsDialog.h"
#include "CoverExtractor.h"

class QApplication;

/**
 * Backend - QML后端桥接类
 * 暴露C++业务逻辑到QML
 */
class Backend : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    // 数据模型
    Q_PROPERTY(ModifierListModel* modifierListModel READ modifierListModel CONSTANT)
    Q_PROPERTY(DownloadedModifierModel* downloadedModifierModel READ downloadedModifierModel CONSTANT)

    // 当前语言
    Q_PROPERTY(int currentLanguage READ currentLanguage NOTIFY languageChanged)

    // 选中的修改器信息
    Q_PROPERTY(QString selectedModifierName READ selectedModifierName NOTIFY selectedModifierChanged)
    Q_PROPERTY(QString selectedModifierVersion READ selectedModifierVersion NOTIFY selectedModifierChanged)
    Q_PROPERTY(int selectedModifierOptionsCount READ selectedModifierOptionsCount NOTIFY selectedModifierChanged)
    Q_PROPERTY(QString selectedModifierLastUpdate READ selectedModifierLastUpdate NOTIFY selectedModifierChanged)
    Q_PROPERTY(QString selectedModifierOptions READ selectedModifierOptions NOTIFY selectedModifierOptionsChanged)
    Q_PROPERTY(QVariantList selectedModifierVersions READ selectedModifierVersions NOTIFY selectedModifierChanged)
    Q_PROPERTY(QString selectedModifierCoverUrl READ selectedModifierCoverUrl NOTIFY selectedModifierChanged)

    // 下载状态
    Q_PROPERTY(bool isDownloading READ isDownloading NOTIFY downloadingChanged)
    Q_PROPERTY(qreal downloadProgress READ downloadProgress NOTIFY downloadProgressChanged)

public:
    explicit Backend(QObject* parent = nullptr);
    ~Backend();

    // 设置 QApplication 引用（用于主题和语言切换）
    void setApplication(QApplication* app) { m_app = app; }

    // 属性访问
    ModifierListModel* modifierListModel() const { return m_modifierListModel; }
    DownloadedModifierModel* downloadedModifierModel() const { return m_downloadedModifierModel; }
    int currentLanguage() const;

    QString selectedModifierName() const;
    QString selectedModifierVersion() const;
    int selectedModifierOptionsCount() const;
    QString selectedModifierLastUpdate() const;
    QString selectedModifierOptions() const;
    QVariantList selectedModifierVersions() const;
    QString selectedModifierCoverUrl() const;

    // 封面提取功能
    Q_INVOKABLE void extractCover();
    Q_PROPERTY(QString selectedModifierCoverPath READ selectedModifierCoverPath NOTIFY coverExtracted)
    QString selectedModifierCoverPath() const { return m_currentCoverPath; }

    bool isDownloading() const { return m_isDownloading; }
    qreal downloadProgress() const { return m_downloadProgress; }

public slots:
    // 搜索功能
    Q_INVOKABLE void searchModifiers(const QString& keyword);
    Q_INVOKABLE void fetchRecentModifiers();
    Q_INVOKABLE void setSortOrder(int sortIndex);

    // 修改器选择
    Q_INVOKABLE void selectModifier(int index);
    Q_INVOKABLE void selectVersion(int versionIndex);

    // 下载功能
    Q_INVOKABLE void downloadModifier(int versionIndex);
    Q_INVOKABLE void openDownloadFolder();

    // 已下载管理
    Q_INVOKABLE void runModifier(int index);
    Q_INVOKABLE void deleteModifier(int index);
    Q_INVOKABLE void checkForUpdates();

    // 设置
    Q_INVOKABLE void setTheme(int themeIndex);
    Q_INVOKABLE void setLanguage(int languageIndex);
    Q_INVOKABLE void openSettings();

signals:
    void languageChanged();
    void selectedModifierChanged();
    void selectedModifierOptionsChanged();
    void downloadingChanged();
    void downloadProgressChanged();
    void searchCompleted();
    void downloadCompleted(bool success);
    void statusMessage(const QString& message);
    void coverExtracted();

private slots:
    void onSearchCompleted(const QList<ModifierInfo>& modifiers);
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onDownloadFinished(bool success);

private:
    void loadDownloadedModifiers();
    void saveDownloadedModifiers();

private:
    QApplication* m_app = nullptr;
    ModifierListModel* m_modifierListModel;
    DownloadedModifierModel* m_downloadedModifierModel;
    
    // 当前选中的修改器
    int m_selectedIndex = -1;
    ModifierInfo m_selectedModifier;
    QString m_selectedOptions;
    int m_selectedVersionIndex = 0;
    
    // 下载状态
    bool m_isDownloading = false;
    qreal m_downloadProgress = 0.0;
    
    // 临时存储已下载修改器列表
    QList<DownloadedModifierInfo> m_downloadedList;
    
    // 封面提取器
    CoverExtractor* m_coverExtractor;
    QString m_currentCoverPath;
};
