#pragma once

#include <QApplication>
#include <QFile>
#include <QDebug>
#include "ConfigManager.h"

/**
 * @brief 主题管理类，负责切换和应用不同的主题样式表
 */
class ThemeManager {
public:
    // 获取单例实例
    static ThemeManager& getInstance() {
        static ThemeManager instance;
        return instance;
    }
    
    // 应用当前主题（从配置中读取）
    void applyCurrentTheme(QApplication& app) {
        ConfigManager::Theme theme = ConfigManager::getInstance().getCurrentTheme();
        applyTheme(app, theme);
    }
    
    // 切换到指定主题
    void switchTheme(QApplication& app, ConfigManager::Theme theme) {
        // 保存主题设置
        ConfigManager::getInstance().setCurrentTheme(theme);
        
        // 应用主题
        applyTheme(app, theme);
    }
    
    // 获取主题名称
    QString getThemeName(ConfigManager::Theme theme) const {
        switch (theme) {
            case ConfigManager::Theme::Light:
                return "浅色主题";
            case ConfigManager::Theme::Win11:
                return "Windows 11主题";
            case ConfigManager::Theme::Classic:
                return "经典主题";
            case ConfigManager::Theme::Colorful:
                return "多彩主题";
            default:
                return "未知主题";
        }
    }

private:
    // 私有构造函数，防止外部创建实例
    ThemeManager() {}
    
    // 私有析构函数
    ~ThemeManager() {}
    
    // 禁用拷贝构造函数和赋值操作符
    ThemeManager(const ThemeManager&) = delete;
    ThemeManager& operator=(const ThemeManager&) = delete;
    
    // 应用指定主题
    void applyTheme(QApplication& app, ConfigManager::Theme theme) {
        QString styleFilePath;
        
        switch (theme) {
            case ConfigManager::Theme::Light:
                styleFilePath = ":/style/main.qss";
                break;
            case ConfigManager::Theme::Win11:
                styleFilePath = ":/style/win11.qss";
                break;
            case ConfigManager::Theme::Classic:
                styleFilePath = ":/style/classic.qss";
                break;
            case ConfigManager::Theme::Colorful:
                styleFilePath = ":/style/colorful.qss";
                break;
            default:
                styleFilePath = ":/style/main.qss";
                break;
        }
          QFile styleFile(styleFilePath);
        if (styleFile.open(QFile::ReadOnly | QFile::Text)) {
            QString styleSheet = QLatin1String(styleFile.readAll());
            app.setStyleSheet(styleSheet);
            qDebug() << "已应用" << getThemeName(theme) << "样式表，文件路径:" << styleFilePath;
            qDebug() << "样式表大小:" << styleSheet.length() << "字符";
            styleFile.close();
        } else {
            qDebug() << "无法打开样式表文件:" << styleFilePath;
            qDebug() << "错误信息:" << styleFile.errorString();
            
            // 列出可用的资源文件
            QDir resourceDir(":/");
            qDebug() << "资源根目录是否存在:" << resourceDir.exists();
            
            QDir styleDir(":/style");
            qDebug() << "样式目录是否存在:" << styleDir.exists();
            if (styleDir.exists()) {
                QStringList styleFiles = styleDir.entryList(QStringList() << "*.qss", QDir::Files);
                qDebug() << "可用的样式文件:" << styleFiles;
            }
            
            // 如果无法加载样式表，回退到浅色主题
            if (theme != ConfigManager::Theme::Light) {
                qDebug() << "回退到浅色主题";
                applyTheme(app, ConfigManager::Theme::Light);
            }
        }
    }
}; 