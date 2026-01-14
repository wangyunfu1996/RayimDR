#include "QtLogger.h"

#include <QFile>
#include <QDir>
#include <QMessageBox>
#include <QDateTime>
#include <QTextStream>
#include <QThread>
#include <QMutex>
#include <QApplication>

#include <DbgHelp.h>
#pragma comment(lib, "dbghelp.lib")

void QtLogger::initialize()
{
	// 设置 Qt 日志格式并安装消息处理器
	//setMessagePattern();
	installMessageHandler();

	// 注册 Windows 未处理异常回调，生成 dump
	SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)applicationCrashHandler);

	qDebug() << "日志模块已初始化";
	qInfo() << "日志模块已初始化";
	qWarning() << "日志模块已初始化";
	qCritical() << "日志模块已初始化";
}

LONG WINAPI QtLogger::applicationCrashHandler(EXCEPTION_POINTERS* pException)
{
	// 创建 Dump 文件
	HANDLE hDumpFile = CreateFile(L"XRayMEMORY.DMP", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hDumpFile != INVALID_HANDLE_VALUE) {
		MINIDUMP_EXCEPTION_INFORMATION dumpInfo;
		dumpInfo.ExceptionPointers = pException;
		dumpInfo.ThreadId = GetCurrentThreadId();
		dumpInfo.ClientPointers = TRUE;
		MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hDumpFile, MiniDumpNormal, &dumpInfo, NULL, NULL);
		CloseHandle(hDumpFile);
	}

	// 弹窗提示并退出
	EXCEPTION_RECORD* record = pException->ExceptionRecord;
	QString errCode(QString::number(record->ExceptionCode, 16));
	QString errAdr(QString::number((uint)record->ExceptionAddress, 16));
	QMessageBox::critical(NULL, "程序崩溃",
		QString("<div><b>对于发生的错误，表示诚挚的歉意</b><br/>错误代码：%1<br/>错误地址：%2</div>").arg(errCode).arg(errAdr),
		QMessageBox::Ok);

	return EXCEPTION_EXECUTE_HANDLER;
}

void QtLogger::setMessagePattern()
{
	qSetMessagePattern("[%{time yyyy-MM-dd hh:mm:ss.zzz}] "
		"[%{if-debug}D%{endif}%{if-info}I%{endif}%{if-warning}W%{endif}%{if-critical}C%{endif}%{if-fatal}F%{endif}] "
		"[%{threadid}] "
		"[%{file}:%{line} - %{function}] "
		"%{message}");
}

void QtLogger::installMessageHandler()
{
	qInstallMessageHandler(customMessageHandler);
}

void QtLogger::customMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
	static QMutex mutex;
	QMutexLocker locker(&mutex);

	// 级别映射
	QString level = "UNKNOWN";
	switch (type) {
	case QtDebugMsg: level = "DEBUG"; break;
#if QT_VERSION >= QT_VERSION_CHECK(5, 5, 0)
	case QtInfoMsg: level = "INFO"; break;
#endif
	case QtWarningMsg: level = "WARN"; break;
	case QtCriticalMsg: level = "ERROR"; break;
	case QtFatalMsg: level = "FATAL"; break;
	default: break;
	}

	// 保护 context 字段可能为 nullptr 的情况
	const char* cfile = context.file ? context.file : "";
	const char* cfunc = context.function ? context.function : "";
	QString fileName = QFileInfo(QString::fromUtf8(cfile)).fileName();
	QString funcName = QString::fromUtf8(cfunc);

	QString threadInfo = QString("Thread[0x%1]").arg((quintptr)QThread::currentThreadId(), 0, 16);
	QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
	QString logMessage = QString("[%1] [%2] [%3] [%4:%5] [%6] %7\n")
		.arg(timestamp)
		.arg(threadInfo)
		.arg(level)
		.arg(fileName.isEmpty() ? QString("-") : fileName)
		.arg(context.line)
		.arg(funcName.isEmpty() ? QString("-") : funcName)
		.arg(msg);

	// 输出到 Visual Studio 调试窗口（Windows），否则回退到 stderr
#ifdef Q_OS_WIN
	std::wstring wmsg = logMessage.toStdWString();
	OutputDebugStringW(wmsg.c_str());
#else
	QByteArray out = logMessage.toLocal8Bit();
	fprintf(stderr, "%s", out.constData());
	fflush(stderr);
#endif

	// 日志文件按天生成，若超过大小则轮转
	const qint64 maxSize = 20LL * 1024 * 1024; // 20 MB
	QString logsDir = qApp->applicationDirPath() + "/logs";
	QString today = QDate::currentDate().toString("yyyy-MM-dd");
	QString desiredLogPath = logsDir + "/app-" + today + ".log";

	static QFile logFile;
	static QString currentLogPath;

	// 如果程序退出后，继续写入当前文件
	if (desiredLogPath.startsWith("/logs"))
	{
		desiredLogPath = currentLogPath;
	}

	if (currentLogPath != desiredLogPath) {
		if (logFile.isOpen()) logFile.close();
		QDir().mkpath(logsDir);
		logFile.setFileName(desiredLogPath);
		logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
		currentLogPath = desiredLogPath;
	}

	if (logFile.isOpen()) {
		QFileInfo fi(logFile.fileName());
		if (fi.exists() && fi.size() >= maxSize) {
			// 轮转：保留多个备份文件，按序号命名为 base.N.ext（例如 app-YYYY-MM-DD.1.log）
			logFile.close();
			QFileInfo dfi(desiredLogPath);
			QString base = dfi.absolutePath() + "/" + dfi.completeBaseName();
			QString ext = dfi.suffix();

			// 找到第一个可用的序号
			int idx = 1;
			QString rotated;
			for (; idx <= 999; ++idx) {
				rotated = QString("%1.%2.%3").arg(base).arg(idx).arg(ext); // base.idx.ext
				if (!QFile::exists(rotated)) break;
			}

			if (idx > 999) {
				// 备份过多，删除最旧的（序号1），然后使用序号1
				QString oldest = QString("%1.%2.%3").arg(base).arg(1).arg(ext);
				QFile::remove(oldest);
				rotated = oldest;
			}

			QFile::rename(fi.absoluteFilePath(), rotated);

			// 重新打开新的日志文件
			logFile.setFileName(desiredLogPath);
			logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
		}

		if (logFile.isOpen()) {
			QTextStream stream(&logFile);
			stream.setCodec("UTF-8");
			stream << logMessage;
			stream.flush();
		}
	}

	// 致命错误立即终止进程
	if (type == QtFatalMsg) {
		abort();
	}
}
