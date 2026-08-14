#include <QGuiApplication>
#include <QCoreApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QDebug>
#include <QDir>
#include <QTranslator>

#ifdef Q_OS_WIN
#include <QOperatingSystemVersion>
#include <QQuickWindow>
#include <qt_windows.h>
#include <dwmapi.h>
#endif

#include "Backend.h"
#include "ModifierListModel.h"
#include "DownloadedModifierModel.h"
#include "ThemeManager.h"
#include "LanguageManager.h"
#include "GameMappingManager.h"
#include "ConfigManager.h"
#include "Logger.h"

#ifdef Q_OS_WIN
namespace {

void applyWindows11RoundedCorners(QObject* rootObject)
{
    if (QOperatingSystemVersion::current() < QOperatingSystemVersion::Windows11) {
        return;
    }

    auto* window = qobject_cast<QQuickWindow*>(rootObject);
    if (!window) {
        LOG_WARN() << "Cannot enable rounded corners: QML root object is not a window";
        return;
    }

    const auto windowHandle = reinterpret_cast<HWND>(window->winId());
    if (!windowHandle) {
        LOG_WARN() << "Cannot enable rounded corners: native window handle is unavailable";
        return;
    }

    const DWM_WINDOW_CORNER_PREFERENCE preference = DWMWCP_ROUND;
    const HRESULT result = DwmSetWindowAttribute(
        windowHandle,
        DWMWA_WINDOW_CORNER_PREFERENCE,
        &preference,
        sizeof(preference));
    if (FAILED(result)) {
        LOG_WARN() << "Failed to enable Windows 11 rounded corners:" << result;
    }
}

} // namespace
#endif

int main(int argc, char *argv[])
{
    // Use QGuiApplication (pure QML app)
    QGuiApplication app(argc, argv);
    Logger::install();

    // Keep app data under "%AppData%\\FLiNG Downloader" instead of
    // "%AppData%\\Sqhh99\\FLiNG Downloader".
    QCoreApplication::setOrganizationName("");
    QCoreApplication::setOrganizationDomain("");
    QCoreApplication::setApplicationName("FLiNG Downloader");
#ifdef FLING_APP_VERSION
    QCoreApplication::setApplicationVersion(QStringLiteral(FLING_APP_VERSION));
#else
    QCoreApplication::setApplicationVersion(QStringLiteral("0.0.0-dev"));
#endif

    try {
        LOG_DEBUG() << "Application initializing...";
        
        // Qt 6 QML module handles resources; no manual Q_INIT_RESOURCE needed
        LOG_DEBUG() << "Resources handled by QML module";
        
        // Verify resource system
        QDir resourceRoot(":/");
        if (resourceRoot.exists()) {
            QStringList entries = resourceRoot.entryList();
            LOG_DEBUG() << "Available resource dirs:" << entries;
        } else {
            LOG_DEBUG() << "Resource system initialization failed";
        }
        
        // Apply current language
        LanguageManager::getInstance().applyCurrentLanguage(app);
        
        // Set QuickControls2 style
        QQuickStyle::setStyle("Basic");
        
        // Initialize game mapping manager
        LOG_DEBUG() << "Initializing game mapping manager...";
        if (GameMappingManager::getInstance().initialize()) {
            LOG_DEBUG() << "Game mapping manager initialized";
        } else {
            LOG_DEBUG() << "Game mapping manager failed, Chinese search may be limited";
        }
        
        // Register QML types
        qmlRegisterType<ModifierListModel>("FLiNGDownloader", 1, 0, "ModifierListModel");
        qmlRegisterType<DownloadedModifierModel>("FLiNGDownloader", 1, 0, "DownloadedModifierModel");
        
        // Create Backend instance
        Backend* backend = new Backend(&app);
        backend->setApplication(&app);
        
        // Create QML engine
        QQmlApplicationEngine engine;
        
        // Set QQmlEngine reference (for language switch refresh)
        backend->setQmlEngine(&engine);
        
        // Add import path for QML module
        engine.addImportPath("qrc:/");
        
        // Expose theme index to QML (for ThemeProvider initialization)
        int currentTheme = static_cast<int>(ConfigManager::getInstance().getCurrentTheme());
        engine.rootContext()->setContextProperty("initialTheme", currentTheme);
        engine.rootContext()->setContextProperty("Log", new LogFacade(&app));
        
        // Set required property initial values
        engine.setInitialProperties({{"backend", QVariant::fromValue(backend)}});
        
        // Load main QML file
        const QUrl url(QStringLiteral("qrc:/qml/Main.qml"));
        
        QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                         &app, [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl) {
                LOG_DEBUG() << "QML load failed";
                QCoreApplication::exit(-1);
            }
        }, Qt::QueuedConnection);

        engine.load(url);
        
        if (engine.rootObjects().isEmpty()) {
            LOG_DEBUG() << "No QML objects loaded";
            return -1;
        }

#ifdef Q_OS_WIN
        applyWindows11RoundedCorners(engine.rootObjects().constFirst());
#endif

        LOG_DEBUG() << "Application initialized, starting event loop";
        
        // Run application
        return app.exec();
    } 
    catch (const std::exception& e) {
        LOG_DEBUG() << "Exception:" << e.what();
        return 1;
    } 
    catch (...) {
        LOG_DEBUG() << "Unknown exception";
        return 1;
    }
}
