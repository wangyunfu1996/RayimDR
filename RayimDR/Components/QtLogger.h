#pragma once

#include <Windows.h>
#include <QString>
#include <QDebug>
#include <QRecursiveMutex>
#include <QFile>

class QtLogger
{
public:
    static void initialize();
    static void setMessagePattern();
    static void installMessageHandler();
    static QString getLogsDir();
    static bool cleanupLogs();

private:
    static void customMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg);
    static LONG WINAPI applicationCrashHandler(EXCEPTION_POINTERS* pException);

    // 日志文件管理
    static void openLogFile();
    static bool rotateCurrentLogFile();
    static void flushLogFile();

    // 私有成员
    static QRecursiveMutex logMutex;  // 改为递归互斥锁
    static QString currentLogPath;
    static QFile* currentLogFile;
    static const qint64 maxLogFileSize;
    static bool initialized;
};
