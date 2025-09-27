#include "ModifierManager.h"
#include "NetworkManager.h"
#include "ConfigManager.h"
#include <QDebug>
#include <QApplication>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include "FileSystem.h"
#include "UpdateManager.h"
#include "DownloadManager.h"
#include "ModifierInfoManager.h"
#include "SearchManager.h"

ModifierManager::ModifierManager(QObject* parent)
    : QObject(parent)
{
    loadDownloadedModifiers();
}

ModifierManager::~ModifierManager()
{
    saveDownloadedModifiers();
}

ModifierManager& ModifierManager::getInstance()
{
    static ModifierManager instance;
    return instance;
}

void ModifierManager::searchModifiers(const QString& searchTerm, ModifierFoundCallback callback)
{
    qDebug() << "ModifierManager::searchModifiers - 搜索修改器：" << (searchTerm.isEmpty() ? "(空字符串，将加载热门列表)" : searchTerm);
    
    // 如果传入了空字符串且有回调函数，调用SearchManager中的loadFeaturedModifiers方法
    if (searchTerm.isEmpty()) {
        qDebug() << "检测到空搜索，将调用SearchManager.searchModifiers获取热门修改器";
        
        // 确保回调函数不为空
        if (!callback) {
            qDebug() << "错误：回调函数为空，无法返回结果";
            return;
        }
    }
    
    // 使用SearchManager进行搜索
    SearchManager::getInstance().searchModifiers(searchTerm, 
        [this, callback, searchTerm](const QList<ModifierInfo>& modifiers) {
            qDebug() << "SearchManager返回结果，获取到" << modifiers.size() << "个修改器";
            
            // 将结果保存到ModifierManager中
            m_modifierList = modifiers;
            
            // 打印前5个修改器名称进行调试
            if (!m_modifierList.isEmpty()) {
                qDebug() << "修改器列表前5项:";
                for (int i = 0; i < qMin(5, m_modifierList.size()); i++) {
                    qDebug() << "  " << (i+1) << ". " << m_modifierList[i].name << " (" << m_modifierList[i].gameVersion << ")";
                }
            } else {
                qDebug() << "警告：修改器列表为空！";
            }
            
            // 调用回调函数
            if (callback) {
                qDebug() << "正在调用回调函数传递结果...";
                callback(m_modifierList);
                qDebug() << "回调函数执行完成";
            } else {
                qDebug() << "错误：回调函数为空，无法返回结果";
            }
        }
    );
}

void ModifierManager::getModifierDetail(const QString& url, ModifierDetailCallback callback)
{
    qDebug() << "获取修改器详情：" << url;
    
    // 从URL中提取修改器名称 - 使用ModifierInfoManager
    QString modifierName = ModifierInfoManager::getInstance().extractNameFromUrl(url);
    if (modifierName.isEmpty()) {
        modifierName = QObject::tr("未知修改器");
    }
    
    // 使用NetworkManager发送请求
    NetworkManager::getInstance().sendGetRequest(
        url,
        [this, modifierName, callback, url](const QByteArray& data, bool success) {
            if (success) {
                // 使用ModifierParser解析响应数据
                ModifierInfo* modifier = ModifierParser::parseModifierDetailHTML(data.toStdString(), modifierName);
                modifier->url = url; // 保存URL以便重新获取
                
                // 使用ModifierInfoManager格式化修改器信息
                modifier->name = ModifierInfoManager::getInstance().formatModifierName(modifier->name);
                
                // 格式化版本号
                for (int i = 0; i < modifier->versions.size(); ++i) {
                    QString formattedVersion = ModifierInfoManager::getInstance().formatVersionString(modifier->versions[i].first);
                    modifier->versions[i].first = formattedVersion;
                }
                
                callback(modifier);
            } else {
                qDebug() << "获取修改器详情失败";
                
                // 创建一个基本的修改器信息作为备用
                ModifierInfo* modifier = new ModifierInfo();
                modifier->name = modifierName;
                modifier->url = url;
                callback(modifier);
            }
        }
    );
}

void ModifierManager::downloadModifier(const ModifierInfo& modifier, 
                                      const QString& version, 
                                      const QString& savePath,
                                      ModifierDownloadFinishedCallback callback,
                                      DLProgressCallback progressCallback)
{    // 使用DownloadManager下载修改器
    DownloadManager::getInstance().downloadModifier(
        modifier,
        version,
        savePath,
        [this, callback, version, savePath](bool success, const QString& errorMsg, const QString& actualPath, const ModifierInfo& modifier, bool isArchive) {
            if (success) {
                // 添加到已下载修改器列表 - 使用实际的文件路径
                addDownloadedModifier(modifier, version, actualPath);
            }
            
            if (callback) {
                callback(success, errorMsg, actualPath, modifier, isArchive);
            }
        },
        progressCallback
    );
}

QList<DownloadedModifierInfo> ModifierManager::getDownloadedModifiers() const
{
    return m_downloadedModifiers;
}

void ModifierManager::addDownloadedModifier(const ModifierInfo& info, const QString& version, const QString& filePath)
{
    // 确保名称非空
    QString modifierName = info.name;
    if (modifierName.isEmpty()) {
        // 使用FileSystem获取文件名
        QFileInfo fileInfo = FileSystem::getInstance().getFileInfo(filePath);
        modifierName = fileInfo.baseName();
    }
    
    // 检查是否已存在
    for (int i = 0; i < m_downloadedModifiers.size(); ++i) {
        if (m_downloadedModifiers[i].name == modifierName && m_downloadedModifiers[i].version == version) {
            // 更新现有条目
            m_downloadedModifiers[i].filePath = filePath;
            m_downloadedModifiers[i].downloadDate = QDateTime::currentDateTime();
            m_downloadedModifiers[i].gameVersion = info.gameVersion;
            m_downloadedModifiers[i].optionsCount = info.optionsCount;
            
            // 确保URL非空且更新现有URL
            if (!info.url.isEmpty()) {
                m_downloadedModifiers[i].url = info.url;
            }
            
            m_downloadedModifiers[i].hasUpdate = false;
            
            // 保存
            saveDownloadedModifiers();
            return;
        }
    }
    
    // 创建新条目
    DownloadedModifierInfo newInfo;
    newInfo.name = modifierName;
    newInfo.version = version;
    newInfo.filePath = filePath;
    newInfo.downloadDate = QDateTime::currentDateTime();
    newInfo.gameVersion = info.gameVersion;
    newInfo.optionsCount = info.optionsCount;
    newInfo.url = info.url;
    newInfo.hasUpdate = false;
    
    // 添加到列表
    m_downloadedModifiers.append(newInfo);
    
    // 保存
    saveDownloadedModifiers();
}

void ModifierManager::removeDownloadedModifier(int index)
{
    if (index < 0 || index >= m_downloadedModifiers.size()) {
        return;
    }
    
    // 删除文件 - 使用FileSystem
    FileSystem::getInstance().deleteFile(m_downloadedModifiers[index].filePath);
    
    // 从列表中删除
    m_downloadedModifiers.removeAt(index);
    
    // 保存
    saveDownloadedModifiers();
}

void ModifierManager::checkForUpdates(int index, std::function<void(bool)> callback)
{
    if (index >= 0 && index < m_downloadedModifiers.size()) {
        // 检查单个修改器的更新
        if (m_downloadedModifiers[index].url.isEmpty()) {
            qDebug() << "修改器URL为空，无法检查更新";
            if (callback) callback(false);
            return;
        }
        
        // 使用UpdateManager检查更新
        UpdateManager::getInstance().checkModifierUpdate(
            m_downloadedModifiers[index].url,
            m_downloadedModifiers[index].version,
            m_downloadedModifiers[index].gameVersion,
            [this, index, callback](bool hasUpdate) {
                // 保存更新状态
                m_downloadedModifiers[index].hasUpdate = hasUpdate;
                
                // 保存下载列表
                saveDownloadedModifiers();
                
                if (callback) callback(hasUpdate);
            }
        );
    } else {
        // 检查所有修改器的更新
        bool hasAnyUrl = false;
        
        for (int i = 0; i < m_downloadedModifiers.size(); ++i) {
            if (!m_downloadedModifiers[i].url.isEmpty()) {
                hasAnyUrl = true;
                break;
            }
        }
        
        if (callback) callback(hasAnyUrl);
    }
}

void ModifierManager::batchCheckForUpdates(UpdateProgressCallback progressCallback, 
                                          std::function<void(int)> completedCallback)
{
    // 构建需要检查的修改器列表
    QList<std::tuple<int, QString, QString, QString>> modifiersToCheck;
    
    for (int i = 0; i < m_downloadedModifiers.size(); ++i) {
        if (!m_downloadedModifiers[i].url.isEmpty()) {
            modifiersToCheck.append(std::make_tuple(
                i, 
                m_downloadedModifiers[i].url,
                m_downloadedModifiers[i].version,
                m_downloadedModifiers[i].gameVersion
            ));
        }
    }
    
    // 使用UpdateManager进行批量检查
    UpdateManager::getInstance().batchCheckUpdates(
        modifiersToCheck,
        [this, progressCallback](int current, int total, bool hasUpdates) {
            // 转发进度回调
            if (progressCallback) {
                progressCallback(current, total, hasUpdates);
            }
        },
        [this, completedCallback](int updatesCount) {
            // 保存下载列表
            saveDownloadedModifiers();
            
            // 转发完成回调
            if (completedCallback) {
                completedCallback(updatesCount);
            }
        }
    );
}

void ModifierManager::setModifierUpdateStatus(int index, bool hasUpdate)
{
    if (index >= 0 && index < m_downloadedModifiers.size()) {
        m_downloadedModifiers[index].hasUpdate = hasUpdate;
        saveDownloadedModifiers();
    }
}

void ModifierManager::sortModifierList(SortType sortType)
{
    switch (sortType) {
        case SortType::ByUpdateDate:
            std::sort(m_modifierList.begin(), m_modifierList.end(), [](const ModifierInfo &a, const ModifierInfo &b) {
                return a.lastUpdate > b.lastUpdate; // 降序排列，最近的在前面
            });
            break;
            
        case SortType::ByName:
            std::sort(m_modifierList.begin(), m_modifierList.end(), [](const ModifierInfo &a, const ModifierInfo &b) {
                return a.name.compare(b.name, Qt::CaseInsensitive) < 0; // 升序排列
            });
            break;
            
        case SortType::ByOptionsCount:
            std::sort(m_modifierList.begin(), m_modifierList.end(), [](const ModifierInfo &a, const ModifierInfo &b) {
                return a.optionsCount > b.optionsCount; // 降序排列，选项多的在前面
            });
            break;
    }
}

QList<ModifierInfo> ModifierManager::getSortedModifierList() const
{
    return m_modifierList;
}

void ModifierManager::saveDownloadedModifiers()
{
    // 使用FileSystem获取应用数据目录，并确保目录存在
    QString dataDir = FileSystem::getInstance().getAppDataDirectory();
    QString settingsPath = dataDir + "/downloaded_modifiers.ini";
    
    QSettings settings(settingsPath, QSettings::IniFormat);
    settings.beginWriteArray("modifiers");
    
    for (int i = 0; i < m_downloadedModifiers.size(); ++i) {
        settings.setArrayIndex(i);
        const DownloadedModifierInfo& info = m_downloadedModifiers.at(i);
        
        settings.setValue("name", info.name);
        settings.setValue("version", info.version);
        settings.setValue("filePath", info.filePath);
        settings.setValue("downloadDate", info.downloadDate);
        settings.setValue("gameVersion", info.gameVersion);
        settings.setValue("optionsCount", info.optionsCount);
        settings.setValue("url", info.url);
    }
    
    settings.endArray();
    settings.sync();
    
    qDebug() << "已下载修改器信息已保存到:" << settingsPath;
}

void ModifierManager::loadDownloadedModifiers()
{
    m_downloadedModifiers.clear();
    
    // 使用FileSystem获取应用数据目录
    QString dataDir = FileSystem::getInstance().getAppDataDirectory();
    QString settingsPath = dataDir + "/downloaded_modifiers.ini";
    
    // 检查文件是否存在
    if (!FileSystem::getInstance().fileExists(settingsPath)) {
        qDebug() << "已下载修改器配置文件不存在，使用默认值";
        return;
    }
    
    QSettings settings(settingsPath, QSettings::IniFormat);
    int size = settings.beginReadArray("modifiers");
    
    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        
        DownloadedModifierInfo info;
        info.name = settings.value("name").toString();
        info.version = settings.value("version").toString();
        info.filePath = settings.value("filePath").toString();
        info.downloadDate = settings.value("downloadDate").toDateTime();
        info.gameVersion = settings.value("gameVersion").toString();
        info.optionsCount = settings.value("optionsCount").toInt();
        info.url = settings.value("url").toString();
        info.hasUpdate = false; // 初始化为无更新
        
        // 检查文件是否存在 - 使用FileSystem
        if (FileSystem::getInstance().fileExists(info.filePath)) {
            m_downloadedModifiers.append(info);
        }
    }
    
    settings.endArray();
    
    qDebug() << "已加载" << m_downloadedModifiers.size() << "个已下载修改器，配置文件:" << settingsPath;
}

QList<ModifierInfo> ModifierManager::filterModifiersByKeyword(const QString& keyword) const
{
    // 使用ModifierInfoManager过滤修改器
    return ModifierInfoManager::getInstance().searchModifiersByKeyword(m_modifierList, keyword);
}

bool ModifierManager::exportModifierToFile(const ModifierInfo& info, const QString& filePath)
{
    // 使用ModifierInfoManager导出修改器信息为JSON
    QString json = ModifierInfoManager::getInstance().exportToJson(info);
    
    // 保存到文件
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "无法打开文件进行写入:" << filePath;
        return false;
    }
    
    QTextStream out(&file);
    out << json;
    file.close();
    
    return true;
}

ModifierInfo ModifierManager::importModifierFromFile(const QString& filePath)
{
    // 从文件读取JSON
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "无法打开文件进行读取:" << filePath;
        return ModifierInfo();
    }
    
    QTextStream in(&file);
    QString json = in.readAll();
    file.close();
    
    // 使用ModifierInfoManager从JSON导入修改器信息
    return ModifierInfoManager::getInstance().importFromJson(json);
}

// 直接设置修改器列表
void ModifierManager::setModifierList(const QList<ModifierInfo>& modifiers)
{
    m_modifierList = modifiers;
    qDebug() << "ModifierManager: 修改器列表已由外部设置，共" << m_modifierList.size() << "个修改器";
} 