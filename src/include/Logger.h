#pragma once

#include <QDateTime>
#include <QDebug>
#include <QMutex>
#include <QObject>
#include <QString>
#include <QtGlobal>

/**
 * @brief Process-wide logger that formats Qt and QML messages
 *
 * Format: [yyyy-MM-dd HH:mm:ss.zzz] [LEVEL] message
 */
class Logger {
public:
    enum class Level {
        Debug,
        Info,
        Warning,
        Error
    };

    static Logger& instance();

    // Install the Qt message handler. Call once after QCoreApplication exists.
    static void install();

    static QString levelName(Level level);
    static Level levelFromQt(QtMsgType type);
    static QString formatLine(Level level,
                              const QString& message,
                              const QDateTime& timestamp = QDateTime::currentDateTime());

    void log(Level level, const QString& message);

private:
    Logger() = default;
    ~Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void writeLine(const QString& line);
    static void qtMessageHandler(QtMsgType type,
                                 const QMessageLogContext& context,
                                 const QString& message);

    QMutex m_mutex;
};

class LogStream {
public:
    explicit LogStream(Logger::Level level)
        : m_level(level)
        , m_debug(&m_buffer)
    {
    }

    ~LogStream()
    {
        Logger::instance().log(m_level, m_buffer.trimmed());
    }

    LogStream(const LogStream&) = delete;
    LogStream& operator=(const LogStream&) = delete;

    template<typename T>
    LogStream& operator<<(const T& value)
    {
        m_debug << value;
        return *this;
    }

private:
    Logger::Level m_level;
    QString m_buffer;
    QDebug m_debug;
};

class LogFacade : public QObject {
    Q_OBJECT
public:
    explicit LogFacade(QObject* parent = nullptr)
        : QObject(parent)
    {
    }

    Q_INVOKABLE void debug(const QString& message)
    {
        Logger::instance().log(Logger::Level::Debug, message);
    }

    Q_INVOKABLE void info(const QString& message)
    {
        Logger::instance().log(Logger::Level::Info, message);
    }

    Q_INVOKABLE void warn(const QString& message)
    {
        Logger::instance().log(Logger::Level::Warning, message);
    }

    Q_INVOKABLE void error(const QString& message)
    {
        Logger::instance().log(Logger::Level::Error, message);
    }
};

#define LOG_DEBUG() LogStream(Logger::Level::Debug)
#define LOG_INFO() LogStream(Logger::Level::Info)
#define LOG_WARN() LogStream(Logger::Level::Warning)
#define LOG_ERROR() LogStream(Logger::Level::Error)
