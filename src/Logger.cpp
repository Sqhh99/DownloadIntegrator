#include "Logger.h"

#include <cstdio>
#include <cstdlib>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

Logger& Logger::instance()
{
    static Logger logger;
    return logger;
}

void Logger::install()
{
#ifdef Q_OS_WIN
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    qInstallMessageHandler(&Logger::qtMessageHandler);
}

QString Logger::levelName(Level level)
{
    switch (level) {
        case Level::Debug:
            return QStringLiteral("DEBUG");
        case Level::Info:
            return QStringLiteral("INFO");
        case Level::Warning:
            return QStringLiteral("WARN");
        case Level::Error:
            return QStringLiteral("ERROR");
    }
    return QStringLiteral("DEBUG");
}

Logger::Level Logger::levelFromQt(QtMsgType type)
{
    switch (type) {
        case QtDebugMsg:
            return Level::Debug;
        case QtInfoMsg:
            return Level::Info;
        case QtWarningMsg:
            return Level::Warning;
        case QtCriticalMsg:
        case QtFatalMsg:
            return Level::Error;
    }
    return Level::Debug;
}

QString Logger::formatLine(Level level, const QString& message, const QDateTime& timestamp)
{
    return QStringLiteral("[%1] [%2] %3")
        .arg(timestamp.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz")),
             levelName(level),
             message);
}

void Logger::log(Level level, const QString& message)
{
    writeLine(formatLine(level, message));
}

void Logger::writeLine(const QString& line)
{
    QMutexLocker locker(&m_mutex);
    const QByteArray utf8 = line.toUtf8();
    std::fwrite(utf8.constData(), 1, static_cast<size_t>(utf8.size()), stderr);
    std::fwrite("\n", 1, 1, stderr);
    std::fflush(stderr);
}

void Logger::qtMessageHandler(QtMsgType type,
                              const QMessageLogContext& context,
                              const QString& message)
{
    Q_UNUSED(context)
    instance().writeLine(formatLine(levelFromQt(type), message));
    if (type == QtFatalMsg) {
        std::abort();
    }
}
