#include "translator.h"
#include <iostream>
#include <sstream>
#include <chrono>
#include <thread>
#include <ctime>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// HTTP响应回调函数
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// URL编码函数（全局函数）
std::string globalUrlEncode(const std::string& str) {
    CURL* curl = curl_easy_init();
    if (!curl) return str;
    
    char* encoded = curl_easy_escape(curl, str.c_str(), str.length());
    std::string result(encoded);
    curl_free(encoded);
    curl_easy_cleanup(curl);
    return result;
}

// AppWorlds翻译器实现
AppWorldsTranslator::AppWorldsTranslator() 
    : baseUrl("https://translate.appworlds.cn")
    , requestInterval(2.1)
    , lastRequestTime(0) {
    
    headers["User-Agent"] = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36";
    headers["Accept"] = "application/json, text/plain, */*";
    headers["Accept-Language"] = "zh-CN,zh;q=0.9,en;q=0.8";
    headers["Content-Type"] = "application/x-www-form-urlencoded";
}

void AppWorldsTranslator::waitForRateLimit() {
    auto now = std::chrono::system_clock::now();
    auto nowTime = std::chrono::system_clock::to_time_t(now);
    
    double timeSinceLast = difftime(nowTime, lastRequestTime);
    if (timeSinceLast < requestInterval) {
        double waitTime = requestInterval - timeSinceLast;
        std::cout << "等待 " << waitTime << " 秒以满足频率限制..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds((int)(waitTime * 1000)));
    }
      lastRequestTime = nowTime;
}

std::string AppWorldsTranslator::urlEncode(const std::string& str) {
    return globalUrlEncode(str);
}

std::string AppWorldsTranslator::httpPost(const std::string& url, const std::string& data) {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;
    
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        
        // 设置请求头
        struct curl_slist* headerList = nullptr;
        for (const auto& header : headers) {
            std::string headerStr = header.first + ": " + header.second;
            headerList = curl_slist_append(headerList, headerStr.c_str());
        }
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerList);
        
        res = curl_easy_perform(curl);
        curl_slist_free_all(headerList);
        curl_easy_cleanup(curl);
    }
    
    return readBuffer;
}

TranslationResult AppWorldsTranslator::translate(const std::string& text, 
                                               const std::string& fromLang, 
                                               const std::string& toLang) {
    TranslationResult result;
    result.original = text;
    result.fromLang = fromLang;
    result.toLang = toLang;
    
    if (text.empty()) {
        result.error = "文本不能为空";
        return result;
    }
    
    if (text.length() > 255) {
        result.error = "文本长度超过255字符限制，当前长度：" + std::to_string(text.length());
        return result;
    }
    
    waitForRateLimit();
    
    try {
        // 构建POST数据
        std::string postData = "text=" + urlEncode(text) + 
                              "&from=" + urlEncode(fromLang) + 
                              "&to=" + urlEncode(toLang);
        
        std::cout << "正在翻译: " << text.substr(0, 50) << (text.length() > 50 ? "..." : "") << std::endl;
        std::cout << "翻译方向: " << fromLang << " -> " << toLang << std::endl;
        
        std::string response = httpPost(baseUrl, postData);
        std::cout << "响应内容: " << response.substr(0, 200) << (response.length() > 200 ? "..." : "") << std::endl;
        
        if (!response.empty()) {
            try {
                json jsonResponse = json::parse(response);
                result.apiResponse = response;
                
                if (jsonResponse.contains("code") && jsonResponse["code"] == 200) {
                    if (jsonResponse.contains("data")) {
                        result.translated = jsonResponse["data"].get<std::string>();
                        result.success = true;
                    }
                } else {
                    std::string errorMsg = jsonResponse.contains("msg") ? 
                                         jsonResponse["msg"].get<std::string>() : "未知错误";
                    result.error = "API返回错误: " + errorMsg;
                }
            } catch (const json::exception& e) {
                result.error = "JSON解析失败: " + std::string(e.what());
            }
        } else {
            result.error = "HTTP请求失败或响应为空";
        }
    } catch (const std::exception& e) {
        result.error = "请求异常: " + std::string(e.what());
    }
    
    return result;
}

std::vector<TranslationResult> AppWorldsTranslator::batchTranslate(
    const std::vector<std::string>& texts,
    const std::string& fromLang,
    const std::string& toLang) {
    
    std::vector<TranslationResult> results;
    
    for (size_t i = 0; i < texts.size(); ++i) {
        std::cout << "进度: " << (i + 1) << "/" << texts.size() << std::endl;
        TranslationResult result = translate(texts[i], fromLang, toLang);
        results.push_back(result);
        
        if (result.success) {
            std::cout << "✓ 翻译成功: " << result.translated.substr(0, 50) 
                     << (result.translated.length() > 50 ? "..." : "") << std::endl;
        } else {
            std::cout << "✗ 翻译失败: " << result.error << std::endl;
        }
    }
    
    return results;
}

bool AppWorldsTranslator::testApi() {
    std::cout << "=== 测试AppWorlds翻译API ===" << std::endl;
    
    std::vector<std::pair<std::string, std::string>> testCases = {
        {"你好世界", "中文转英文"},
        {"Hello World", "英文转中文"},
        {"艾尔登法环", "游戏名称翻译"},
        {"人工智能技术正在快速发展", "长句翻译"}
    };
    
    for (size_t i = 0; i < testCases.size(); ++i) {
        std::cout << "\n测试 " << (i + 1) << ": " << testCases[i].second << std::endl;
        std::cout << "原文: " << testCases[i].first << std::endl;
        
        TranslationResult result = translate(testCases[i].first, "zh-CN", "en");
        
        if (result.success) {
            std::cout << "译文: " << result.translated << std::endl;
            std::cout << "✓ 测试成功" << std::endl;
        } else {
            std::cout << "✗ 测试失败: " << result.error << std::endl;
        }
        
        std::cout << std::string(50, '-') << std::endl;
    }
    
    return true;
}

// SuApi翻译器实现
SuApiTranslator::SuApiTranslator() 
    : baseUrl("https://suapi.net/api/text/translate")
    , requestInterval(1.0)
    , lastRequestTime(0) {
    
    headers["User-Agent"] = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36";
    headers["Accept"] = "application/json, text/plain, */*";
    headers["Accept-Language"] = "zh-CN,zh;q=0.9,en;q=0.8";
}

void SuApiTranslator::waitForRateLimit() {
    auto now = std::chrono::system_clock::now();
    auto nowTime = std::chrono::system_clock::to_time_t(now);
    
    double timeSinceLast = difftime(nowTime, lastRequestTime);
    if (timeSinceLast < requestInterval) {
        double waitTime = requestInterval - timeSinceLast;
        std::cout << "等待 " << waitTime << " 秒..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds((int)(waitTime * 1000)));
    }
      lastRequestTime = nowTime;
}

std::string SuApiTranslator::urlEncode(const std::string& str) {
    return globalUrlEncode(str);
}

std::string SuApiTranslator::httpGet(const std::string& url) {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;
    
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
        
        // 设置请求头
        struct curl_slist* headerList = nullptr;
        for (const auto& header : headers) {
            std::string headerStr = header.first + ": " + header.second;
            headerList = curl_slist_append(headerList, headerStr.c_str());
        }
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerList);
        
        res = curl_easy_perform(curl);
        curl_slist_free_all(headerList);
        curl_easy_cleanup(curl);
    }
    
    return readBuffer;
}

TranslationResult SuApiTranslator::translate(const std::string& text, 
                                           const std::string& fromLang, 
                                           const std::string& toLang) {
    TranslationResult result;
    result.original = text;
    result.fromLang = fromLang;
    result.toLang = toLang;
    
    if (text.empty()) {
        result.error = "文本不能为空";
        return result;
    }
    
    waitForRateLimit();
    
    try {
        // 构建GET URL
        std::string url = baseUrl + "?to=" + urlEncode(toLang) + "&text[]=" + urlEncode(text);
        
        std::cout << "正在翻译: " << text.substr(0, 50) << (text.length() > 50 ? "..." : "") << std::endl;
        std::cout << "目标语言: " << toLang << std::endl;
        std::cout << "请求URL: " << url << std::endl;
        
        std::string response = httpGet(url);
        std::cout << "响应内容: " << response.substr(0, 500) << (response.length() > 500 ? "..." : "") << std::endl;
        
        if (!response.empty()) {
            try {
                json jsonResponse = json::parse(response);
                result.apiResponse = response;
                
                // 检查是否是数组格式
                if (jsonResponse.is_array() && !jsonResponse.empty()) {
                    result.translated = jsonResponse[0].get<std::string>();
                    result.success = true;
                }                // 检查是否是对象格式
                else if (jsonResponse.is_object()) {
                    // 处理SuApi的嵌套JSON格式
                    if (jsonResponse.contains("code") && jsonResponse["code"] == 200 && 
                        jsonResponse.contains("data") && jsonResponse["data"].is_array() &&
                        !jsonResponse["data"].empty()) {
                        
                        auto dataArray = jsonResponse["data"];
                        if (dataArray[0].contains("translations") && dataArray[0]["translations"].is_array() &&
                            !dataArray[0]["translations"].empty()) {
                            
                            auto translations = dataArray[0]["translations"];
                            if (translations[0].contains("text")) {
                                result.translated = translations[0]["text"].get<std::string>();
                                result.success = true;
                            } else {
                                result.error = "翻译结果中缺少text字段";
                            }
                        } else {
                            result.error = "翻译结果格式错误：缺少translations字段";
                        }
                    }
                    // 其他格式
                    else if (jsonResponse.contains("result")) {
                        result.translated = jsonResponse["result"].get<std::string>();
                        result.success = true;
                    } else if (jsonResponse.contains("data") && jsonResponse["data"].is_string()) {
                        result.translated = jsonResponse["data"].get<std::string>();
                        result.success = true;
                    } else if (jsonResponse.contains("translation")) {
                        result.translated = jsonResponse["translation"].get<std::string>();
                        result.success = true;
                    } else {
                        result.error = "无法从响应中提取翻译结果";
                    }
                }else {
                    result.error = "意外的响应格式";
                }
            } catch (const json::exception& e) {
                // 如果不是JSON，尝试直接使用文本内容
                if (!response.empty()) {
                    result.translated = response;
                    result.success = true;
                } else {
                    result.error = "JSON解析失败且响应为空: " + std::string(e.what());
                }
            }
        } else {
            result.error = "HTTP请求失败或响应为空";
        }
    } catch (const std::exception& e) {
        result.error = "请求异常: " + std::string(e.what());
    }
    
    return result;
}

std::vector<TranslationResult> SuApiTranslator::batchTranslate(
    const std::vector<std::string>& texts,
    const std::string& fromLang,
    const std::string& toLang) {
    
    std::vector<TranslationResult> results;
    
    // 尝试批量请求
    waitForRateLimit();
    
    try {
        std::string url = baseUrl + "?to=" + urlEncode(toLang);
        for (const auto& text : texts) {
            url += "&text[]=" + urlEncode(text);
        }
        
        std::cout << "批量翻译 " << texts.size() << " 条文本到 " << toLang << std::endl;
        
        std::string response = httpGet(url);
        std::cout << "响应内容: " << response.substr(0, 500) << (response.length() > 500 ? "..." : "") << std::endl;
        
        if (!response.empty()) {
            try {
                json jsonResponse = json::parse(response);
                
                if (jsonResponse.is_array()) {
                    // 如果返回数组，按顺序匹配
                    for (size_t i = 0; i < texts.size(); ++i) {
                        TranslationResult result;
                        result.original = texts[i];
                        result.fromLang = fromLang;
                        result.toLang = toLang;
                        
                        if (i < jsonResponse.size()) {
                            result.translated = jsonResponse[i].get<std::string>();
                            result.success = true;
                        } else {
                            result.error = "结果数量不匹配";
                        }
                        results.push_back(result);
                    }
                    return results;
                }
            } catch (const json::exception& e) {
                std::cout << "批量翻译JSON解析失败，回退到单个翻译" << std::endl;
            }
        }
    } catch (const std::exception& e) {
        std::cout << "批量翻译异常，回退到单个翻译: " << e.what() << std::endl;
    }
    
    // 回退到单个翻译
    for (const auto& text : texts) {
        results.push_back(translate(text, fromLang, toLang));
    }
    
    return results;
}

bool SuApiTranslator::testApi() {
    std::cout << "=== 测试SuApi翻译API ===" << std::endl;
    
    std::vector<std::pair<std::string, std::string>> testCases = {
        {"你好世界", "中文转英文"},
        {"Hello World", "英文转中文"},
        {"艾尔登法环", "游戏名称翻译"},
        {"人工智能技术正在快速发展", "长句翻译"}
    };
    
    std::cout << "\n=== 单条翻译测试 ===" << std::endl;
    for (size_t i = 0; i < testCases.size(); ++i) {
        std::cout << "\n测试 " << (i + 1) << ": " << testCases[i].second << std::endl;
        std::cout << "原文: " << testCases[i].first << std::endl;
        
        TranslationResult result = translate(testCases[i].first, "auto", "en");
        
        if (result.success) {
            std::cout << "译文: " << result.translated << std::endl;
            std::cout << "✓ 测试成功" << std::endl;
        } else {
            std::cout << "✗ 测试失败: " << result.error << std::endl;
        }
        
        std::cout << std::string(50, '-') << std::endl;
    }
    
    // 批量翻译测试
    std::cout << "\n=== 批量翻译测试 ===" << std::endl;
    std::vector<std::string> batchTexts = {"你好", "世界", "测试"};
    auto batchResults = batchTranslate(batchTexts, "auto", "en");
    
    for (const auto& result : batchResults) {
        std::cout << "原文: " << result.original << std::endl;
        if (result.success) {
            std::cout << "译文: " << result.translated << std::endl;
            std::cout << "✓ 批量测试成功" << std::endl;
        } else {
            std::cout << "✗ 批量测试失败: " << result.error << std::endl;
        }
        std::cout << std::string(30, '-') << std::endl;
    }
    
    return true;
}

// 翻译器工厂实现
std::unique_ptr<ITranslator> TranslatorFactory::createTranslator(TranslatorType type) {
    switch (type) {
        case TranslatorType::APP_WORLDS:
            return std::make_unique<AppWorldsTranslator>();
        case TranslatorType::SU_API:
            return std::make_unique<SuApiTranslator>();
        default:
            return nullptr;
    }
}
