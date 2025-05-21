#include "UpdateManager.h"
#include "ModifierParser.h"
#include <QDebug>
#include <QRegularExpression>

UpdateManager::UpdateManager(QObject* parent)
    : QObject(parent)
{
}

void UpdateManager::checkModifierUpdate(const QString& url, const QString& currentVersion, 
                                       const QString& currentGameVersion,
                                       std::function<void(bool)> callback)
{
    if (url.isEmpty()) {
        qDebug() << "更新检查URL为空，无法检查更新";
        if (callback) {
            callback(false);
        }
        return;
    }

    // 使用NetworkManager获取修改器详情页
    NetworkManager::getInstance().sendGetRequest(
        url,
        [this, currentVersion, currentGameVersion, callback](const QByteArray& data, bool success) {
            if (success) {
                // 解析页面内容
                ModifierInfo* modifier = ModifierParser::parseModifierDetailHTML(data.toStdString(), "");
                
                if (modifier) {
                    // 比较游戏版本
                    bool hasUpdate = false;
                    
                    // 如果游戏版本不同，视为有更新
                    if (modifier->gameVersion != currentGameVersion) {
                        hasUpdate = true;
                    }
                    
                    // 比较修改器版本 - 如果有版本号，进行版本号比较
                    if (!modifier->versions.isEmpty() && !currentVersion.isEmpty()) {
                        QString latestVersion = modifier->versions.first().first;
                        if (isNewerVersion(currentVersion, latestVersion)) {
                            hasUpdate = true;
                        }
                    }
                    
                    delete modifier;
                    
                    if (callback) {
                        callback(hasUpdate);
                    }
                } else {
                    qDebug() << "解析修改器详情失败";
                    if (callback) {
                        callback(false);
                    }
                }
            } else {
                qDebug() << "获取修改器详情页失败";
                if (callback) {
                    callback(false);
                }
            }
        }
    );
}

void UpdateManager::batchCheckUpdates(const QList<std::tuple<int, QString, QString, QString>>& modifiers,
                                      UpdateProgressCallback progressCallback,
                                      std::function<void(int)> completedCallback)
{
    // 检查是否有修改器需要更新
    if (modifiers.isEmpty()) {
        if (completedCallback) {
            completedCallback(0);
        }
        return;
    }
    
    // 创建计数器来跟踪完成的检查
    int* completedChecks = new int(0);
    int* updatesFound = new int(0);
    QList<int>* updatedIndices = new QList<int>();
    
    // 对每个修改器进行检查
    for (const auto& modifierInfo : modifiers) {
        int index = std::get<0>(modifierInfo);
        const QString& url = std::get<1>(modifierInfo);
        const QString& version = std::get<2>(modifierInfo);
        const QString& gameVersion = std::get<3>(modifierInfo);
        
        // 检查URL是否有效
        if (url.isEmpty()) {
            (*completedChecks)++;
            
            // 更新进度
            if (progressCallback) {
                progressCallback(*completedChecks, modifiers.size(), !updatedIndices->isEmpty());
            }
            
            // 检查是否全部完成
            if (*completedChecks >= modifiers.size()) {
                if (completedCallback) {
                    completedCallback(*updatesFound);
                }
                
                // 清理内存
                delete completedChecks;
                delete updatesFound;
                delete updatedIndices;
            }
            
            continue;
        }
        
        // 检查更新
        checkModifierUpdate(
            url, version, gameVersion,
            [this, index, completedChecks, updatesFound, updatedIndices, modifiers, progressCallback, completedCallback](bool hasUpdate) {
                // 更新计数器
                if (hasUpdate) {
                    (*updatesFound)++;
                    updatedIndices->append(index);
                }
                
                (*completedChecks)++;
                
                // 更新进度
                if (progressCallback) {
                    progressCallback(*completedChecks, modifiers.size(), !updatedIndices->isEmpty());
                }
                
                // 检查是否全部完成
                if (*completedChecks >= modifiers.size()) {
                    if (completedCallback) {
                        completedCallback(*updatesFound);
                    }
                    
                    // 清理内存
                    delete completedChecks;
                    delete updatesFound;
                    delete updatedIndices;
                }
            }
        );
    }
}

bool UpdateManager::isNewerVersion(const QString& oldVersion, const QString& newVersion)
{
    // 如果版本号相同，无更新
    if (oldVersion == newVersion) {
        return false;
    }
    
    // 如果任一版本为空，则认为有更新
    if (oldVersion.isEmpty() || newVersion.isEmpty()) {
        return true;
    }
    
    // 提取版本号中的数字部分
    QRegularExpression re("(\\d+)");
    QRegularExpressionMatchIterator oldMatches = re.globalMatch(oldVersion);
    QRegularExpressionMatchIterator newMatches = re.globalMatch(newVersion);
    
    // 比较每个数字部分
    while (oldMatches.hasNext() && newMatches.hasNext()) {
        QRegularExpressionMatch oldMatch = oldMatches.next();
        QRegularExpressionMatch newMatch = newMatches.next();
        
        int oldNum = oldMatch.captured(1).toInt();
        int newNum = newMatch.captured(1).toInt();
        
        if (newNum > oldNum) {
            return true;
        } else if (newNum < oldNum) {
            return false;
        }
    }
    
    // 如果新版本有更多的数字部分，视为较新
    if (newMatches.hasNext()) {
        return true;
    }
    
    // 默认情况下假设没有更新
    return false;
} 