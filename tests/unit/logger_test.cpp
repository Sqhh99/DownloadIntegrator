#include <gtest/gtest.h>

#include <QDateTime>
#include <QRegularExpression>

#include "Logger.h"

class LoggerTest : public ::testing::Test
{
};

TEST_F(LoggerTest, FormatLineUsesTimestampLevelAndMessage)
{
    const QDateTime timestamp = QDateTime::fromString(
        QStringLiteral("2026-08-14 16:32:05.123"),
        QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"));

    const QString line = Logger::formatLine(
        Logger::Level::Info,
        QStringLiteral("Application initializing..."),
        timestamp);

    EXPECT_EQ(line, QStringLiteral("[2026-08-14 16:32:05.123] [INFO] Application initializing..."));
}

TEST_F(LoggerTest, FormatLineTimestampIsParseable)
{
    const QString line = Logger::formatLine(Logger::Level::Debug, QStringLiteral("hello"));
    const QRegularExpression pattern(
        QStringLiteral(R"(^\[(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d{3})\] \[DEBUG\] hello$)"));
    const QRegularExpressionMatch match = pattern.match(line);
    ASSERT_TRUE(match.hasMatch());

    const QDateTime parsed = QDateTime::fromString(
        match.captured(1),
        QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"));
    EXPECT_TRUE(parsed.isValid());
}

TEST_F(LoggerTest, LevelFromQtMapsMessageTypes)
{
    EXPECT_EQ(Logger::levelFromQt(QtDebugMsg), Logger::Level::Debug);
    EXPECT_EQ(Logger::levelFromQt(QtInfoMsg), Logger::Level::Info);
    EXPECT_EQ(Logger::levelFromQt(QtWarningMsg), Logger::Level::Warning);
    EXPECT_EQ(Logger::levelFromQt(QtCriticalMsg), Logger::Level::Error);
    EXPECT_EQ(Logger::levelFromQt(QtFatalMsg), Logger::Level::Error);
}

TEST_F(LoggerTest, LevelNameMatchesFormatToken)
{
    EXPECT_EQ(Logger::levelName(Logger::Level::Debug), QStringLiteral("DEBUG"));
    EXPECT_EQ(Logger::levelName(Logger::Level::Info), QStringLiteral("INFO"));
    EXPECT_EQ(Logger::levelName(Logger::Level::Warning), QStringLiteral("WARN"));
    EXPECT_EQ(Logger::levelName(Logger::Level::Error), QStringLiteral("ERROR"));
}

TEST_F(LoggerTest, StreamMacroFormatsWithoutGoingThroughQtDebug)
{
    const QString line = Logger::formatLine(
        Logger::Level::Warn,
        QStringLiteral("SearchManager: Failed to fetch recently updated modifiers"),
        QDateTime::fromString(QStringLiteral("2026-08-14 16:32:05.140"),
                              QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz")));
    EXPECT_EQ(line,
              QStringLiteral("[2026-08-14 16:32:05.140] [WARN] SearchManager: Failed to fetch recently updated modifiers"));
}
