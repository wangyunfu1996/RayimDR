#pragma once

#include "iraydetector_global.h"
#include <Windows.h>
#include <QString>
#include <QDebug>

class IRAYDETECTOR_EXPORT QtLogger
{
public:
	static void initialize();
	static void setMessagePattern();
	static void installMessageHandler();

private:
	static void customMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg);
	static LONG WINAPI applicationCrashHandler(EXCEPTION_POINTERS* pException);
};
