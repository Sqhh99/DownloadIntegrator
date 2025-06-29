#include "DownloadIntegrator.h"

#include <QApplication>
#include <QMessageBox>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QStyleFactory>
#include "ThemeManager.h"
#include "LanguageManager.h"
#include "GameMappingManager.h"

#ifdef _WIN32
#include <windows.h>
#endif

// 添加DLL搜索路径
void addDllSearchPaths() {
#ifdef _WIN32
    // 添加vcpkg安装的DLL目录到搜索路径
    QString vcpkgBinPath = "D:/vcpkg/installed/x64-windows/bin";
    QByteArray vcpkgPathBytes = vcpkgBinPath.toLocal8Bit();
    
    // 使用Windows API添加DLL搜索路径
    SetDllDirectoryA(vcpkgPathBytes.constData());

    qDebug() << "已添加DLL搜索路径: " << vcpkgBinPath;
#endif
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
    try {
        qDebug() << "应用程序开始初始化...";
        
        // 首先初始化资源系统（关键！）
        Q_INIT_RESOURCE(resources);
        qDebug() << "✅ 资源系统已初始化";
        
        // 验证资源系统
        QDir resourceRoot(":/");
        if (resourceRoot.exists()) {
            QStringList entries = resourceRoot.entryList();
            qDebug() << "📁 可用资源目录:" << entries;
        } else {
            qDebug() << "❌ 资源系统初始化失败!";
        }
        
        // 添加DLL搜索路径
        addDllSearchPaths();
          // 应用当前语言
        LanguageManager::getInstance().applyCurrentLanguage(a);
        
        // 应用当前主题
        ThemeManager::getInstance().applyCurrentTheme(a);
        
        // 初始化游戏名映射管理器
        qDebug() << "正在初始化游戏名映射管理器...";
        if (GameMappingManager::getInstance().initialize()) {
            qDebug() << "✅ 游戏名映射管理器初始化成功";
        } else {
            qDebug() << "⚠️ 游戏名映射管理器初始化失败，中文搜索功能可能受限";
        }
        
        // 创建主窗口
        DownloadIntegrator w;
        w.show();
        
        qDebug() << "应用程序初始化完成，启动事件循环";
        
        // 运行应用程序
        return a.exec();
    } 
    catch (const std::exception& e) {
        qDebug() << "发生异常:" << e.what();
        QMessageBox::critical(nullptr, "错误", QString("应用程序初始化失败: %1").arg(e.what()));
        return 1;
    } 
    catch (...) {
        qDebug() << "发生未知异常";
        QMessageBox::critical(nullptr, "错误", "应用程序初始化过程中发生未知错误");
        return 1;
    }
}
