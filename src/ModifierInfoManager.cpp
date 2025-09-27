#include "ModifierInfoManager.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <QDebug>

ModifierInfoManager::ModifierInfoManager(QObject* parent)
    : QObject(parent)
{
}

ModifierInfo ModifierInfoManager::createModifierInfo(const QString& name, 
                                                   const QString& gameVersion,
                                                   const QString& url)
{
    ModifierInfo info;
    info.name = formatModifierName(name);
    info.gameVersion = gameVersion;
    info.url = url;
    info.lastUpdate = QDateTime::currentDateTime().toString("yyyy-MM-dd");
    info.optionsCount = 0;
    
    return info;
}

ModifierInfo* ModifierInfoManager::cloneModifierInfo(const ModifierInfo& other)
{
    ModifierInfo* info = new ModifierInfo();
    info->name = other.name;
    info->gameVersion = other.gameVersion;
    info->url = other.url;
    info->lastUpdate = other.lastUpdate;
    info->optionsCount = other.optionsCount;
    info->options = other.options;
    info->versions = other.versions;
    
    return info;
}

QString ModifierInfoManager::extractNameFromUrl(const QString& url)
{
    // 从URL中提取修改器名称
    // 尝试多种URL格式
    QStringList patterns = {
        "/([^/]+)-trainer/?$",          // 原有格式: game-name-trainer
        "/trainer/([^/]+)-trainer/?$",  // FLiNG格式: /trainer/game-name-trainer
        "/trainer/([^/]+)/?$"           // 简化格式: /trainer/game-name
    };
    
    for (const QString& pattern : patterns) {
        QRegularExpression re(pattern);
        QRegularExpressionMatch match = re.match(url);
        
        if (match.hasMatch()) {
            QString name = match.captured(1);
            name.replace('-', ' ');
            name.replace('_', ' ');
            
            // 移除可能的 "trainer" 后缀
            if (name.endsWith(" trainer", Qt::CaseInsensitive)) {
                name.chop(8); // 移除 " trainer"
            }
            
            return formatModifierName(name.trimmed());
        }
    }
    
    return QString();
}

QString ModifierInfoManager::formatModifierName(const QString& name)
{
    if (name.isEmpty()) {
        return name;
    }
    
    // 转换为标题格式（每个单词首字母大写）
    QStringList words = name.split(' ', Qt::SkipEmptyParts);
    for (QString& word : words) {
        if (!word.isEmpty()) {
            word[0] = word[0].toUpper();
        }
    }
    
    return words.join(' ');
}

QString ModifierInfoManager::formatVersionString(const QString& version)
{
    // 确保版本号格式一致
    // 如果版本号只包含数字，添加 v 前缀
    if (QRegularExpression("^\\d+\\.?\\d*$").match(version).hasMatch()) {
        return QString("v%1").arg(version);
    }
    
    // 如果已经有前缀（如 v, ver., version 等），统一为 v
    QString formattedVersion = version;
    QRegularExpression prefixRe("^(v|ver\\.|version|V)\\s*(\\d.*)$");
    QRegularExpressionMatch match = prefixRe.match(version);
    
    if (match.hasMatch()) {
        formattedVersion = QString("v%1").arg(match.captured(2));
    }
    
    return formattedVersion;
}

void ModifierInfoManager::addVersionToModifier(ModifierInfo& info, const QString& version, const QString& url)
{
    // 格式化版本号
    QString formattedVersion = formatVersionString(version);
    
    // 检查是否已存在相同版本
    for (int i = 0; i < info.versions.size(); ++i) {
        if (info.versions[i].first == formattedVersion) {
            // 更新URL
            info.versions[i].second = url;
            return;
        }
    }
    
    // 添加新版本
    info.versions.append(qMakePair(formattedVersion, url));
}

int ModifierInfoManager::compareModifierSimilarity(const ModifierInfo& a, const ModifierInfo& b)
{
    int similarity = 0;
    
    // 比较名称 (50%)
    if (a.name.toLower() == b.name.toLower()) {
        similarity += 50;
    } else {
        // 部分匹配
        QString nameLowerA = a.name.toLower();
        QString nameLowerB = b.name.toLower();
        
        if (nameLowerA.contains(nameLowerB) || nameLowerB.contains(nameLowerA)) {
            similarity += 30;
        } else {
            // 检查单词级别的匹配
            QStringList wordsA = nameLowerA.split(' ', Qt::SkipEmptyParts);
            QStringList wordsB = nameLowerB.split(' ', Qt::SkipEmptyParts);
            
            int matchingWords = 0;
            for (const QString& wordA : wordsA) {
                if (wordA.length() < 3) continue; // 忽略短词
                
                for (const QString& wordB : wordsB) {
                    if (wordB.length() < 3) continue;
                    
                    if (wordA == wordB) {
                        matchingWords++;
                        break;
                    }
                }
            }
            
            if (!wordsA.isEmpty() && !wordsB.isEmpty()) {
                similarity += 20 * matchingWords / qMax(wordsA.size(), wordsB.size());
            }
        }
    }
    
    // 比较游戏版本 (20%)
    if (!a.gameVersion.isEmpty() && !b.gameVersion.isEmpty()) {
        if (a.gameVersion == b.gameVersion) {
            similarity += 20;
        } else if (a.gameVersion.contains(b.gameVersion) || b.gameVersion.contains(a.gameVersion)) {
            similarity += 10;
        }
    }
    
    // 比较URL (30%)
    if (!a.url.isEmpty() && !b.url.isEmpty()) {
        if (a.url == b.url) {
            similarity += 30;
        } else {
            // 提取URL中的关键部分进行比较
            QString keyPartA = extractNameFromUrl(a.url);
            QString keyPartB = extractNameFromUrl(b.url);
            
            if (!keyPartA.isEmpty() && !keyPartB.isEmpty() && keyPartA == keyPartB) {
                similarity += 20;
            }
        }
    }
    
    return similarity;
}

QString ModifierInfoManager::exportToJson(const ModifierInfo& info)
{
    QJsonObject rootObj;
    rootObj["name"] = info.name;
    rootObj["gameVersion"] = info.gameVersion;
    rootObj["url"] = info.url;
    rootObj["lastUpdate"] = info.lastUpdate;
    rootObj["optionsCount"] = info.optionsCount;
    
    // 选项列表
    QJsonArray optionsArray;
    for (const QString& option : info.options) {
        optionsArray.append(option);
    }
    rootObj["options"] = optionsArray;
    
    // 版本列表
    QJsonArray versionsArray;
    for (const auto& version : info.versions) {
        QJsonObject versionObj;
        versionObj["version"] = version.first;
        versionObj["url"] = version.second;
        versionsArray.append(versionObj);
    }
    rootObj["versions"] = versionsArray;
    
    QJsonDocument doc(rootObj);
    return doc.toJson(QJsonDocument::Indented);
}

ModifierInfo ModifierInfoManager::importFromJson(const QString& json)
{
    ModifierInfo info;
    
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    if (doc.isNull() || !doc.isObject()) {
        qWarning() << "无效的JSON格式：" << json;
        return info;
    }
    
    QJsonObject rootObj = doc.object();
    
    // 基本信息
    info.name = rootObj["name"].toString();
    info.gameVersion = rootObj["gameVersion"].toString();
    info.url = rootObj["url"].toString();
    info.lastUpdate = rootObj["lastUpdate"].toString();
    info.optionsCount = rootObj["optionsCount"].toInt();
    
    // 选项列表
    QJsonArray optionsArray = rootObj["options"].toArray();
    for (const QJsonValue& value : optionsArray) {
        info.options.append(value.toString());
    }
    
    // 版本列表
    QJsonArray versionsArray = rootObj["versions"].toArray();
    for (const QJsonValue& value : versionsArray) {
        QJsonObject versionObj = value.toObject();
        info.versions.append(qMakePair(
            versionObj["version"].toString(),
            versionObj["url"].toString()
        ));
    }
    
    return info;
}

QStringList ModifierInfoManager::convertHtmlOptionsToPlainText(const QString& htmlOptions)
{
    QStringList result;
    
    // 使用正则表达式移除HTML标签
    QRegularExpression htmlTagRe("<.*?>");
    QString plainText = htmlOptions;
    plainText.replace(htmlTagRe, "");
    
    // 处理HTML实体
    plainText.replace("&nbsp;", " ");
    plainText.replace("&amp;", "&");
    plainText.replace("&lt;", "<");
    plainText.replace("&gt;", ">");
    plainText.replace("&quot;", "\"");
    
    // 分割为行
    QStringList lines = plainText.split('\n', Qt::SkipEmptyParts);
    
    // 清理每一行
    for (QString& line : lines) {
        line = line.trimmed();
        if (!line.isEmpty()) {
            result.append(line);
        }
    }
    
    return result;
}

QList<ModifierInfo> ModifierInfoManager::searchModifiersByKeyword(const QList<ModifierInfo>& modifiers, const QString& keyword)
{
    QList<ModifierInfo> results;
    
    if (keyword.isEmpty()) {
        return modifiers; // 如果关键词为空，返回所有修改器
    }
    
    QString lowerKeyword = keyword.toLower();
    
    for (const ModifierInfo& info : modifiers) {
        // 在名称中搜索
        if (info.name.toLower().contains(lowerKeyword)) {
            results.append(info);
            continue;
        }
        
        // 在游戏版本中搜索
        if (info.gameVersion.toLower().contains(lowerKeyword)) {
            results.append(info);
            continue;
        }
        
        // 在选项中搜索
        bool foundInOptions = false;
        for (const QString& option : info.options) {
            if (option.toLower().contains(lowerKeyword)) {
                foundInOptions = true;
                break;
            }
        }
        
        if (foundInOptions) {
            results.append(info);
        }
    }
    
    return results;
} 