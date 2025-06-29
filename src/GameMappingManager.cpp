#include "GameMappingManager.h"
#include "translator.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QDebug>
#include <QMutexLocker>
#include <QTimer>
#include <QtConcurrent>
#include <QFuture>

GameMappingManager& GameMappingManager::getInstance()
{
    static GameMappingManager instance;
    return instance;
}

GameMappingManager::GameMappingManager(QObject* parent)
    : QObject(parent), m_initialized(false), m_translationTimer(nullptr)
{
    m_translationTimer = new QTimer(this);
    m_translationTimer->setSingleShot(true);
    connect(m_translationTimer, &QTimer::timeout, this, &GameMappingManager::onTranslationTimerTimeout);
}

bool GameMappingManager::initialize()
{
    QMutexLocker locker(&m_mutex);
    
    if (m_initialized) {
        return true;
    }
    
    qDebug() << "GameMappingManager: 初始化游戏映射数据...";
    
    // 初始化翻译器
    initializeTranslator();
    
    // 加载内置映射
    if (!loadBuiltinMappings()) {
        qWarning() << "GameMappingManager: 加载内置映射失败";
        return false;
    }
    
    // 加载用户自定义映射
    loadUserMappings();
    
    // 加载翻译缓存
    loadTranslationCache();
    
    m_initialized = true;
    qDebug() << "GameMappingManager: 初始化完成，加载了" << m_builtinMappings.size() << "个内置映射";
    
    return true;
}

QString GameMappingManager::translateToEnglish(const QString& chinese)
{
    QMutexLocker locker(&m_mutex);
    
    if (chinese.isEmpty()) {
        return QString();
    }
      // 1. 检查翻译缓存
    if (m_translationCache.contains(chinese)) {
        QString cachedResult = m_translationCache[chinese];
        qDebug() << "GameMappingManager: 从缓存获取翻译:" << chinese << "->" << cachedResult;
        
        // 尝试解析JSON响应
        QString parsedResult = parseTranslationFromJson(cachedResult);
        if (!parsedResult.isEmpty()) {
            return parsedResult;
        }
        
        // 如果解析失败，直接返回原始结果（可能是之前的简单映射）
        return cachedResult;
    }
    
    // 2. 检查用户自定义映射
    if (m_userMappings.contains(chinese)) {
        return m_userMappings[chinese];
    }
    
    // 3. 检查内置映射
    if (m_builtinMappings.contains(chinese)) {
        return m_builtinMappings[chinese].english;
    }
    
    // 4. 检查别名映射
    if (m_aliasToEnglish.contains(chinese)) {
        return m_aliasToEnglish[chinese];
    }
    
    // 5. 部分匹配检查
    for (auto it = m_builtinMappings.begin(); it != m_builtinMappings.end(); ++it) {
        // 检查是否包含关键词
        if (chinese.contains(it.key()) || it.key().contains(chinese)) {
            return it.value().english;
        }
        
        // 检查别名
        for (const QString& alias : it.value().aliases) {
            if (chinese.contains(alias) || alias.contains(chinese)) {
                return it.value().english;
            }
        }
    }
    
    return QString(); // 未找到映射，需要异步翻译
}

void GameMappingManager::translateToEnglishAsync(const QString& chinese, std::function<void(const QString&)> callback)
{
    // 先尝试同步查找
    QString result = translateToEnglish(chinese);
    if (!result.isEmpty()) {
        callback(result);
        return;
    }
    
    // 如果没有找到，添加到异步翻译队列
    QMutexLocker locker(&m_mutex);
    
    if (!m_pendingTranslations.contains(chinese)) {
        m_pendingTranslations.append(chinese);
        m_translationCallbacks[chinese] = callback;
        
        qDebug() << "GameMappingManager: 添加到翻译队列:" << chinese;
        
        // 启动翻译定时器（避免过于频繁的API调用）
        if (!m_translationTimer->isActive()) {
            m_translationTimer->start(1000); // 1秒后开始翻译
        }
    } else {
        // 如果已经在队列中，更新回调
        m_translationCallbacks[chinese] = callback;
    }
}

QString GameMappingManager::fuzzyMatch(const QString& input) const
{
    QMutexLocker locker(&m_mutex);
    
    if (input.isEmpty()) {
        return QString();
    }
    
    QString bestMatch;
    double maxSimilarity = 0.0;
    const double threshold = 0.6; // 相似度阈值
    
    // 检查所有内置映射
    for (auto it = m_builtinMappings.begin(); it != m_builtinMappings.end(); ++it) {
        double similarity = calculateSimilarity(input, it.key());
        if (similarity > maxSimilarity && similarity >= threshold) {
            maxSimilarity = similarity;
            bestMatch = it.value().english;
        }
        
        // 检查别名
        for (const QString& alias : it.value().aliases) {
            similarity = calculateSimilarity(input, alias);
            if (similarity > maxSimilarity && similarity >= threshold) {
                maxSimilarity = similarity;
                bestMatch = it.value().english;
            }
        }
    }
    
    return bestMatch;
}

bool GameMappingManager::containsChinese(const QString& text) const
{
    for (const QChar& ch : text) {
        if (ch.unicode() >= 0x4E00 && ch.unicode() <= 0x9FFF) {
            return true;
        }
    }
    return false;
}

QStringList GameMappingManager::getAllChineseNames() const
{
    QMutexLocker locker(&m_mutex);
    QStringList result = m_builtinMappings.keys();
    result.append(m_userMappings.keys());
    result.append(m_translationCache.keys());
    result.removeDuplicates();
    return result;
}

QStringList GameMappingManager::getAllAliases() const
{
    QMutexLocker locker(&m_mutex);
    
    QStringList allAliases;
    allAliases << m_builtinMappings.keys(); // 添加主要中文名
    
    for (const GameMappingInfo& info : m_builtinMappings) {
        allAliases << info.aliases;
    }
    
    allAliases.removeDuplicates();
    return allAliases;
}

void GameMappingManager::addUserMapping(const QString& chinese, const QString& english)
{
    QMutexLocker locker(&m_mutex);
    m_userMappings[chinese] = english;
    qDebug() << "GameMappingManager: 添加用户映射:" << chinese << "->" << english;
    
    // 自动保存用户映射
    locker.unlock();
    saveUserMappings();
}

void GameMappingManager::addTranslationCache(const QString& chinese, const QString& english)
{
    QMutexLocker locker(&m_mutex);
    m_translationCache[chinese] = english;
    qDebug() << "GameMappingManager: 添加翻译缓存:" << chinese << "->" << english;
}

bool GameMappingManager::loadBuiltinMappings()
{
    // 清空现有映射
    m_builtinMappings.clear();
    m_aliasToEnglish.clear();
    
    // 尝试从资源文件加载JSON映射
    QString filePath = getBuiltinMappingPath();
    QFile file(filePath);
    
    if (!file.exists()) {
        qWarning() << "GameMappingManager: 内置映射文件不存在:" << filePath;
        return loadDefaultMappings(); // 回退到默认映射
    }
    
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "GameMappingManager: 无法打开内置映射文件:" << filePath;
        return loadDefaultMappings(); // 回退到默认映射
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        qWarning() << "GameMappingManager: 内置映射JSON解析错误:" << error.errorString();
        return loadDefaultMappings(); // 回退到默认映射
    }
    
    QJsonObject root = doc.object();
    QJsonObject mappings = root["mappings"].toObject();
    
    // 解析每个游戏映射
    for (auto it = mappings.begin(); it != mappings.end(); ++it) {
        QString chinese = it.key();
        QJsonObject gameObj = it.value().toObject();
        
        GameMappingInfo info;
        info.english = gameObj["english"].toString();
        info.category = gameObj["category"].toString();
        
        // 解析别名
        QJsonArray aliasArray = gameObj["aliases"].toArray();
        for (const auto& alias : aliasArray) {
            info.aliases.append(alias.toString());
        }
        
        m_builtinMappings[chinese] = info;
        
        // 建立别名映射
        for (const QString& alias : info.aliases) {
            m_aliasToEnglish[alias] = info.english;
        }
    }
    
    qDebug() << "GameMappingManager: 从JSON文件加载了" << m_builtinMappings.size() << "个内置映射";
    return true;
}

bool GameMappingManager::loadDefaultMappings()
{
    // 创建基础的内置映射作为回退（仅包含最基础的映射）
    m_builtinMappings.clear();
    m_aliasToEnglish.clear();
    
    qWarning() << "GameMappingManager: JSON文件加载失败，使用最小化默认映射";
    
    // 只保留最基础的几个映射作为紧急回退
    QMap<QString, GameMappingInfo> defaultMappings;
    
    // 最重要的游戏映射（确保即使JSON文件损坏也能工作）
    GameMappingInfo stellarblade;
    stellarblade.english = "Stellar Blade";
    stellarblade.aliases = QStringList{"剑", "星之剑"};
    stellarblade.category = "action";
    defaultMappings["剑星"] = stellarblade;
    
    GameMappingInfo eldenring;
    eldenring.english = "Elden Ring";
    eldenring.aliases = QStringList{"老头环", "法环"};
    eldenring.category = "action";
    defaultMappings["艾尔登法环"] = eldenring;
      GameMappingInfo blackmyth;
    blackmyth.english = "Black Myth: Wukong";
    blackmyth.aliases = QStringList{"悟空", "黑神话"};
    blackmyth.category = "action";
    defaultMappings["黑神话悟空"] = blackmyth;
    
    GameMappingInfo assassinscreed;
    assassinscreed.english = "Assassin's Creed";
    assassinscreed.aliases = QStringList{"AC", "刺客信条系列"};
    assassinscreed.category = "action";
    defaultMappings["刺客信条"] = assassinscreed;
    
    GameMappingInfo frostpunk2;
    frostpunk2.english = "Frostpunk 2";
    frostpunk2.aliases = QStringList{"冰雪朋克2", "极地朋克2"};
    frostpunk2.category = "strategy";
    defaultMappings["冰汽时代2"] = frostpunk2;
    
    // 将默认映射添加到主映射中
    m_builtinMappings = defaultMappings;
    
    // 建立别名映射
    for (auto it = m_builtinMappings.begin(); it != m_builtinMappings.end(); ++it) {
        for (const QString& alias : it.value().aliases) {
            m_aliasToEnglish[alias] = it.value().english;
        }
    }
    
    qDebug() << "GameMappingManager: 使用最小化默认映射，加载了" << m_builtinMappings.size() << "个基础映射";
    return true;
}

bool GameMappingManager::loadUserMappings()
{
    QString filePath = getUserMappingPath();
    QFile file(filePath);
    
    if (!file.exists()) {
        qDebug() << "GameMappingManager: 用户映射文件不存在，跳过加载";
        return true; // 不存在是正常的
    }
    
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "GameMappingManager: 无法打开用户映射文件:" << filePath;
        return false;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        qWarning() << "GameMappingManager: 用户映射JSON解析错误:" << error.errorString();
        return false;
    }
    
    QJsonObject mappings = doc.object();
    m_userMappings.clear();
    
    for (auto it = mappings.begin(); it != mappings.end(); ++it) {
        m_userMappings[it.key()] = it.value().toString();
        qDebug() << "GameMappingManager: 加载用户映射:" << it.key() << "->" << it.value().toString();
    }
    
    return true;
}

bool GameMappingManager::loadTranslationCache()
{
    QString filePath = getTranslationCachePath();
    QFile file(filePath);
    
    if (!file.exists()) {
        qDebug() << "GameMappingManager: 翻译缓存文件不存在，跳过加载";
        return true; // 不存在是正常的
    }
    
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "GameMappingManager: 无法打开翻译缓存文件:" << filePath;
        return false;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        qWarning() << "GameMappingManager: 翻译缓存JSON解析错误:" << error.errorString();
        return false;
    }
    
    QJsonObject obj = doc.object();
    m_translationCache.clear();
    
    // 修复：检查并清理包含原始JSON的缓存项
    bool needsCleanup = false;
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        QString value = it.value().toString();
        
        // 检查是否是原始JSON响应（包含"code":200格式）
        if (value.contains("{\"code\":") && value.contains("\"data\":")) {
            qDebug() << "GameMappingManager: 检测到原始JSON缓存，尝试解析:" << it.key();
            
            // 尝试从JSON中提取真正的翻译结果
            QString parsedResult = parseTranslationFromJson(value);
            if (!parsedResult.isEmpty() && parsedResult != value) {
                m_translationCache[it.key()] = parsedResult;
                qDebug() << "GameMappingManager: 修复翻译缓存:" << it.key() << "->" << parsedResult;
                needsCleanup = true;
            } else {
                qWarning() << "GameMappingManager: 无法修复翻译缓存，删除:" << it.key();
                needsCleanup = true;
            }
        } else {
            // 正常的翻译结果
            m_translationCache[it.key()] = value;
            qDebug() << "GameMappingManager: 加载翻译缓存:" << it.key() << "->" << value;
        }
    }
    
    // 如果有修复，保存清理后的缓存
    if (needsCleanup) {
        qDebug() << "GameMappingManager: 翻译缓存已修复，保存清理后的版本";
        saveTranslationCache();
    }
    
    return true;
}

void GameMappingManager::saveUserMappings()
{
    QMutexLocker locker(&m_mutex);
    
    QString filePath = getUserMappingPath();
    QDir dir = QFileInfo(filePath).dir();
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    QJsonObject mappings;
    for (auto it = m_userMappings.begin(); it != m_userMappings.end(); ++it) {
        mappings[it.key()] = it.value();
    }
    
    QJsonDocument doc(mappings);
    
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file.write(doc.toJson());
        file.close();
        qDebug() << "GameMappingManager: 用户映射已保存到:" << filePath;
    } else {
        qWarning() << "GameMappingManager: 无法保存用户映射到:" << filePath;
    }
}

void GameMappingManager::saveTranslationCache()
{
    QMutexLocker locker(&m_mutex);
    
    QString filePath = getTranslationCachePath();
    QDir dir = QFileInfo(filePath).dir();
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    QJsonObject cache;
    for (auto it = m_translationCache.begin(); it != m_translationCache.end(); ++it) {
        cache[it.key()] = it.value();
    }
    
    QJsonDocument doc(cache);
    
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file.write(doc.toJson());
        file.close();
        qDebug() << "GameMappingManager: 翻译缓存已保存到:" << filePath;
    } else {
        qWarning() << "GameMappingManager: 无法保存翻译缓存到:" << filePath;
    }
}

double GameMappingManager::calculateSimilarity(const QString& str1, const QString& str2) const
{
    // 简单的相似度计算
    if (str1 == str2) return 1.0;
    if (str1.isEmpty() || str2.isEmpty()) return 0.0;
    
    // 计算包含关系
    if (str1.contains(str2) || str2.contains(str1)) {
        return 0.8;
    }
    
    // 计算字符匹配度
    int matches = 0;
    int maxLen = qMax(str1.length(), str2.length());
    int minLen = qMin(str1.length(), str2.length());
    
    for (int i = 0; i < minLen; ++i) {
        if (str1[i] == str2[i]) {
            matches++;
        }
    }
    
    return static_cast<double>(matches) / maxLen;
}

QString GameMappingManager::getUserMappingPath() const
{
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return QDir(appDataPath).filePath("user_game_mappings.json");
}

QString GameMappingManager::getTranslationCachePath() const
{
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return QDir(appDataPath).filePath("translation_cache.json");
}

QString GameMappingManager::getBuiltinMappingPath() const
{
    return ":/game_mappings.json";
}

void GameMappingManager::initializeTranslator()
{
    // 初始化翻译器（默认使用SuApi，因为它看起来更稳定）
    m_translator = TranslatorFactory::createTranslator(TranslatorFactory::TranslatorType::SU_API);
    
    if (!m_translator) {
        qWarning() << "GameMappingManager: 翻译器初始化失败";
    } else {
        qDebug() << "GameMappingManager: 翻译器初始化成功";
    }
}

void GameMappingManager::onTranslationTimerTimeout()
{
    if (m_pendingTranslations.isEmpty()) {
        return;
    }
    
    QString chinese = m_pendingTranslations.takeFirst();
    performTranslation(chinese);
}

void GameMappingManager::performTranslation(const QString& chinese)
{
    if (!m_translator) {
        qWarning() << "GameMappingManager: 翻译器未初始化";
        emit translationFailed(chinese, "翻译器未初始化");
        return;
    }
      qDebug() << "GameMappingManager: 开始翻译" << chinese;
    
    // 在后台线程中执行翻译
    auto future = QtConcurrent::run([this, chinese]() {
        std::string chineseStd = chinese.toStdString();
        TranslationResult result = m_translator->translate(chineseStd, "zh-CN", "en");
        
        if (result.success && !result.translated.empty()) {
            QString english = QString::fromStdString(result.translated);
            
            // 简单的后处理：首字母大写
            if (!english.isEmpty()) {
                english[0] = english[0].toUpper();
            }
            
            // 添加到翻译缓存
            addTranslationCache(chinese, english);
            saveTranslationCache();
            
            qDebug() << "GameMappingManager: 翻译成功:" << chinese << "->" << english;
            
            // 回调处理
            QMutexLocker locker(&m_mutex);
            if (m_translationCallbacks.contains(chinese)) {
                auto callback = m_translationCallbacks[chinese];
                m_translationCallbacks.remove(chinese);
                locker.unlock();
                
                // 在主线程中执行回调
                QMetaObject::invokeMethod(this, [callback, english]() {
                    callback(english);
                }, Qt::QueuedConnection);
            }
            
            emit translationCompleted(chinese, english);
        } else {
            QString error = QString::fromStdString(result.error);
            qWarning() << "GameMappingManager: 翻译失败:" << chinese << "错误:" << error;
            
            // 移除回调
            QMutexLocker locker(&m_mutex);
            m_translationCallbacks.remove(chinese);
            
            emit translationFailed(chinese, error);
        }
    });
}

bool GameMappingManager::reloadMappings()
{
    QMutexLocker locker(&m_mutex);
    m_initialized = false;
    locker.unlock();
    
    return initialize();
}

QString GameMappingManager::parseTranslationFromJson(const QString& jsonResponse) const
{
    if (jsonResponse.isEmpty()) {
        return QString();
    }
    
    // 如果不是JSON格式，直接返回
    if (!jsonResponse.startsWith("{") && !jsonResponse.startsWith("[")) {
        return jsonResponse;
    }
    
    try {
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(jsonResponse.toUtf8(), &error);
        
        if (error.error != QJsonParseError::NoError) {
            qWarning() << "GameMappingManager: JSON解析错误:" << error.errorString();
            return QString();
        }
        
        QJsonObject obj = doc.object();
        
        // 处理SuAPI格式
        if (obj.contains("data") && obj["data"].isArray()) {
            QJsonArray dataArray = obj["data"].toArray();
            if (!dataArray.isEmpty()) {
                QJsonObject firstData = dataArray[0].toObject();
                if (firstData.contains("translations") && firstData["translations"].isArray()) {
                    QJsonArray translations = firstData["translations"].toArray();
                    if (!translations.isEmpty()) {
                        QJsonObject translation = translations[0].toObject();
                        if (translation.contains("text")) {
                            QString result = translation["text"].toString().trimmed();
                            qDebug() << "GameMappingManager: 从SuAPI JSON解析出翻译结果:" << result;
                            return result;
                        }
                    }
                }
            }
        }
        
        // 处理AppWorlds格式
        if (obj.contains("data") && obj["data"].isString()) {
            QString result = obj["data"].toString().trimmed();
            qDebug() << "GameMappingManager: 从AppWorlds JSON解析出翻译结果:" << result;
            return result;
        }
        
        // 处理其他可能的格式
        if (obj.contains("result")) {
            QString result = obj["result"].toString().trimmed();
            qDebug() << "GameMappingManager: 从result字段解析出翻译结果:" << result;
            return result;
        }
        
        if (obj.contains("translation")) {
            QString result = obj["translation"].toString().trimmed();
            qDebug() << "GameMappingManager: 从translation字段解析出翻译结果:" << result;
            return result;
        }
        
    } catch (const std::exception& e) {
        qWarning() << "GameMappingManager: JSON解析异常:" << e.what();
    }
    
    qWarning() << "GameMappingManager: 无法从JSON中解析翻译结果:" << jsonResponse.left(100);
    return QString();
}
