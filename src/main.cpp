#include "DownloadIntegrator.h"

#include <QApplication>
#include <QMessageBox>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QStyleFactory>
#include "ThemeManager.h"
#include "LanguageManager.h"

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
        
        // 添加DLL搜索路径
        addDllSearchPaths();
        
        // 应用当前语言
        LanguageManager::getInstance().applyCurrentLanguage(a);
        
        // 应用当前主题
        ThemeManager::getInstance().applyCurrentTheme(a);
        
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
