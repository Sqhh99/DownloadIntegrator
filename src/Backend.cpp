#include "Backend.h"
#include <QApplication>
#include <QDesktopServices>
#include <QUrl>
#include <QProcess>
#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QTimer>
#include "FileSystem.h"
#include "ConfigManager.h"
#include "ThemeManager.h"
#include "LanguageManager.h"
#include "DownloadManager.h"
#include <QRegularExpression>

Backend::Backend(QObject* parent)
    : QObject(parent)
    , m_modifierListModel(new ModifierListModel(this))
    , m_downloadedModifierModel(new DownloadedModifierModel(this))
    , m_coverExtractor(new CoverExtractor(this))
{
    qDebug() << "Backend 初始化...";
    loadDownloadedModifiers();
    
    // 初始加载最近更新的修改器
    fetchRecentModifiers();
}

Backend::~Backend()
{
    saveDownloadedModifiers();
}

int Backend::currentLanguage() const
{
    return static_cast<int>(ConfigManager::getInstance().getCurrentLanguage());
}

QString Backend::selectedModifierName() const
{
    return m_selectedModifier.name;
}

QString Backend::selectedModifierVersion() const
{
    return m_selectedModifier.gameVersion;
}

int Backend::selectedModifierOptionsCount() const
{
    return m_selectedModifier.optionsCount;
}

QString Backend::selectedModifierLastUpdate() const
{
    return m_selectedModifier.lastUpdate;
}

QString Backend::selectedModifierOptions() const
{
    return m_selectedOptions;
}

QVariantList Backend::selectedModifierVersions() const
{
    QVariantList versions;
    for (const auto& version : m_selectedModifier.versions) {
        QVariantMap versionMap;
        versionMap["name"] = version.first;  // 版本标识
        versionMap["url"] = version.second;  // 下载链接
        versions.append(versionMap);
    }
    return versions;
}

QString Backend::selectedModifierCoverUrl() const
{
    // 使用修改器的截图URL作为封面
    // screenshotUrl 通常是游戏截图，可用于显示
    return m_selectedModifier.screenshotUrl;
}

void Backend::searchModifiers(const QString& keyword)
{
    qDebug() << "搜索修改器:" << keyword;
    emit statusMessage(tr("正在搜索: %1").arg(keyword));
    
    SearchManager::getInstance().searchModifiers(keyword, this, &Backend::onSearchCompleted);
}

void Backend::fetchRecentModifiers()
{
    qDebug() << "获取最近更新的修改器列表...";
    emit statusMessage(tr("正在加载修改器列表..."));
    
    SearchManager::getInstance().fetchRecentlyUpdatedModifiers(this, &Backend::onSearchCompleted);
}

void Backend::setSortOrder(int sortIndex)
{
    qDebug() << "设置排序方式:" << sortIndex;
    // 根据排序索引更新列表
    // 0: 最近更新, 1: 按名称, 2: 下载次数
}

void Backend::selectModifier(int index)
{
    if (index < 0 || index >= m_modifierListModel->count()) {
        return;
    }
    
    m_selectedIndex = index;
    m_selectedModifier = m_modifierListModel->getModifier(index);
    m_selectedVersionIndex = 0;
    
    qDebug() << "选中修改器:" << m_selectedModifier.name;
    emit selectedModifierChanged();
    
    // 获取修改器详情（选项列表等）
    if (!m_selectedModifier.url.isEmpty()) {
        emit statusMessage(tr("正在加载修改器详情..."));
        
        // 使用 ModifierManager 获取详情
        ModifierManager::getInstance().getModifierDetail(
            m_selectedModifier.url,
            [this](ModifierInfo* modifier) {
                if (modifier) {
                    qDebug() << "修改器详情获取成功:" << modifier->name 
                             << "版本数:" << modifier->versions.size()
                             << "选项数:" << modifier->options.size();
                    
                    // 更新选中的修改器详情
                    m_selectedModifier.versions = modifier->versions;
                    m_selectedModifier.options = modifier->options;
                    m_selectedModifier.optionsCount = modifier->optionsCount;
                    m_selectedModifier.screenshotUrl = modifier->screenshotUrl;
                    m_selectedModifier.description = modifier->description;
                    
                    // 构建选项文本
                    m_selectedOptions = m_selectedModifier.options.join("\n");
                    
                    emit selectedModifierChanged();
                    emit selectedModifierOptionsChanged();
                    emit statusMessage(tr("修改器详情加载完成"));
                    
                    // 自动提取封面
                    extractCover();
                    
                    delete modifier;
                } else {
                    qDebug() << "获取修改器详情失败";
                    emit statusMessage(tr("获取修改器详情失败"));
                }
            }
        );
    }
}

void Backend::selectVersion(int versionIndex)
{
    m_selectedVersionIndex = versionIndex;
    qDebug() << "选中版本:" << versionIndex;
}

void Backend::downloadModifier(int versionIndex)
{
    if (m_selectedModifier.versions.isEmpty()) {
        emit statusMessage(tr("无可用下载版本"));
        return;
    }
    
    int idx = (versionIndex >= 0 && versionIndex < m_selectedModifier.versions.size()) 
              ? versionIndex : 0;
    
    QString versionName = m_selectedModifier.versions.at(idx).first;   // first is version name
    QString downloadDir = ConfigManager::getInstance().getDownloadDirectory();
    QString savePath = downloadDir + "/" + m_selectedModifier.name + "_" + versionName + ".zip";
    
    qDebug() << "开始下载:" << versionName << "到" << savePath;
    m_isDownloading = true;
    m_downloadProgress = 0.0;
    emit downloadingChanged();
    emit statusMessage(tr("正在下载: %1").arg(m_selectedModifier.name));
    
    // 使用 ModifierManager 进行真实下载
    ModifierManager::getInstance().downloadModifier(
        m_selectedModifier,
        versionName,
        savePath,
        // 完成回调 (bool success, const QString& errorMsg, const QString& filePath, const ModifierInfo& modifier, bool isArchive)
        [this, versionName](bool success, const QString& errorMsg, const QString& filePath, const ModifierInfo& modifier, bool isArchive) {
            Q_UNUSED(modifier)
            Q_UNUSED(isArchive)
            
            m_isDownloading = false;
            emit downloadingChanged();
            
            if (success) {
                m_downloadProgress = 1.0;
                emit downloadProgressChanged();
                emit downloadCompleted(true);
                emit statusMessage(tr("下载完成: %1").arg(filePath));
                
                // 添加到已下载列表
                DownloadedModifierInfo downloadedInfo;
                downloadedInfo.name = m_selectedModifier.name;
                downloadedInfo.version = versionName;
                downloadedInfo.gameVersion = m_selectedModifier.gameVersion;
                downloadedInfo.downloadDate = QDateTime::currentDateTime();
                downloadedInfo.filePath = filePath;
                downloadedInfo.url = m_selectedModifier.url;
                
                m_downloadedList.append(downloadedInfo);
                m_downloadedModifierModel->setModifiers(m_downloadedList);
                saveDownloadedModifiers();
                
                qDebug() << "已添加到下载列表:" << downloadedInfo.name;
            } else {
                m_downloadProgress = 0.0;
                emit downloadProgressChanged();
                emit downloadCompleted(false);
                emit statusMessage(tr("下载失败: %1").arg(errorMsg));
            }
        },
        // 进度回调 (int percentage)
        [this](int progress) {
            m_downloadProgress = static_cast<qreal>(progress) / 100.0;
            emit downloadProgressChanged();
        }
    );
}

// 封面提取
void Backend::extractCover()
{
    if (m_selectedModifier.screenshotUrl.isEmpty()) {
        qDebug() << "无修改器截图URL可用";
        return;
    }
    
    qDebug() << "开始提取封面:" << m_selectedModifier.screenshotUrl;
    emit statusMessage(tr("正在提取游戏封面..."));
    
    m_coverExtractor->extractCoverFromTrainerImage(
        m_selectedModifier.screenshotUrl,
        [this](const QPixmap& cover, bool success) {
            if (success && !cover.isNull()) {
                // 缓存封面到文件
                QString gameId = m_selectedModifier.name;
                gameId.replace(QRegularExpression("[^a-zA-Z0-9]"), "_");
                
                // 保存到缓存目录
                QString cachePath = CoverExtractor::getCacheDirectory();
                QString coverFilePath = cachePath + "/" + gameId + ".png";
                
                if (cover.save(coverFilePath, "PNG")) {
                    // 使用 file:// URL 格式供 QML Image 使用
                    m_currentCoverPath = "file:///" + coverFilePath;
                    emit coverExtracted();
                    emit statusMessage(tr("封面提取成功"));
                    
                    qDebug() << "封面提取成功，保存到:" << m_currentCoverPath << "尺寸:" << cover.size();
                } else {
                    qDebug() << "封面保存失败:" << coverFilePath;
                    emit statusMessage(tr("封面保存失败"));
                }
            } else {
                qDebug() << "封面提取失败";
                emit statusMessage(tr("封面提取失败"));
            }
        }
    );
}

void Backend::openDownloadFolder()
{
    QString downloadDir = ConfigManager::getInstance().getDownloadDirectory();
    QDesktopServices::openUrl(QUrl::fromLocalFile(downloadDir));
}

void Backend::runModifier(int index)
{
    if (index < 0 || index >= m_downloadedModifierModel->count()) {
        return;
    }
    
    DownloadedModifierInfo modifier = m_downloadedModifierModel->getModifier(index);
    qDebug() << "运行修改器:" << modifier.filePath;
    
    if (QFile::exists(modifier.filePath)) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(modifier.filePath));
    } else {
        emit statusMessage(tr("文件不存在: %1").arg(modifier.filePath));
    }
}

void Backend::deleteModifier(int index)
{
    if (index < 0 || index >= m_downloadedModifierModel->count()) {
        return;
    }
    
    DownloadedModifierInfo modifier = m_downloadedModifierModel->getModifier(index);
    qDebug() << "删除修改器:" << modifier.name;
    
    // 删除文件
    if (QFile::exists(modifier.filePath)) {
        QFile::remove(modifier.filePath);
    }
    
    // 从模型移除
    m_downloadedModifierModel->removeModifier(index);
    saveDownloadedModifiers();
    
    emit statusMessage(tr("已删除: %1").arg(modifier.name));
}

void Backend::checkForUpdates()
{
    qDebug() << "检查更新...";
    emit statusMessage(tr("正在检查更新..."));
    // TODO: 实现更新检查逻辑
}

void Backend::setTheme(int themeIndex)
{
    if (!m_app) return;
    
    ConfigManager::Theme theme = static_cast<ConfigManager::Theme>(themeIndex);
    ThemeManager::getInstance().switchTheme(*m_app, theme);
    qDebug() << "切换主题:" << themeIndex;
}

void Backend::setLanguage(int languageIndex)
{
    if (!m_app) return;
    
    LanguageManager::Language language = static_cast<LanguageManager::Language>(languageIndex);
    LanguageManager::getInstance().switchLanguage(*m_app, language);
    emit languageChanged();
    qDebug() << "切换语言:" << languageIndex;
}

void Backend::openSettings()
{
    qDebug() << "打开设置对话框";
    // 创建设置对话框
    SettingsDialog* dialog = new SettingsDialog(nullptr);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->exec();
}

void Backend::onSearchCompleted(const QList<ModifierInfo>& modifiers)
{
    qDebug() << "搜索完成，结果数量:" << modifiers.size();
    m_modifierListModel->setModifiers(modifiers);
    emit searchCompleted();
    emit statusMessage(tr("找到 %1 个修改器").arg(modifiers.size()));
}

void Backend::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    if (bytesTotal > 0) {
        m_downloadProgress = static_cast<qreal>(bytesReceived) / bytesTotal;
        emit downloadProgressChanged();
    }
}

void Backend::onDownloadFinished(bool success)
{
    m_isDownloading = false;
    emit downloadingChanged();
    emit downloadCompleted(success);
    
    if (success) {
        emit statusMessage(tr("下载完成"));
    } else {
        emit statusMessage(tr("下载失败"));
    }
}

void Backend::loadDownloadedModifiers()
{
    QString filePath = FileSystem::getInstance().getAppDataDirectory() + "/downloaded_modifiers.json";
    QFile file(filePath);
    
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "无法读取已下载修改器列表";
        return;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    
    if (!doc.isArray()) {
        return;
    }
    
    QList<DownloadedModifierInfo> list;
    QJsonArray array = doc.array();
    
    for (const auto& item : array) {
        QJsonObject obj = item.toObject();
        DownloadedModifierInfo info;
        info.name = obj["name"].toString();
        info.version = obj["version"].toString();
        info.gameVersion = obj["gameVersion"].toString();
        info.downloadDate = QDateTime::fromString(obj["downloadDate"].toString(), Qt::ISODate);
        info.filePath = obj["filePath"].toString();
        info.url = obj["url"].toString();
        list.append(info);
    }
    
    m_downloadedModifierModel->setModifiers(list);
    qDebug() << "已加载" << list.size() << "个已下载修改器";
}

void Backend::saveDownloadedModifiers()
{
    QString filePath = FileSystem::getInstance().getAppDataDirectory() + "/downloaded_modifiers.json";
    QFile file(filePath);
    
    if (!file.open(QIODevice::WriteOnly)) {
        qDebug() << "无法保存已下载修改器列表";
        return;
    }
    
    QJsonArray array;
    for (int i = 0; i < m_downloadedModifierModel->count(); ++i) {
        DownloadedModifierInfo info = m_downloadedModifierModel->getModifier(i);
        QJsonObject obj;
        obj["name"] = info.name;
        obj["version"] = info.version;
        obj["gameVersion"] = info.gameVersion;
        obj["downloadDate"] = info.downloadDate.toString(Qt::ISODate);
        obj["filePath"] = info.filePath;
        obj["url"] = info.url;
        array.append(obj);
    }
    
    QJsonDocument doc(array);
    file.write(doc.toJson());
    file.close();
    
    qDebug() << "已保存" << m_downloadedModifierModel->count() << "个已下载修改器";
}
