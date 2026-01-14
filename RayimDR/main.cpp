#include <qapplication.h>
#include <qscreen.h>
#include <qdebug.h>
#include <qdatetime.h>
#include <qtimer.h>

#include "ElaApplication.h"

#include "UI/MainWindow.h"

#include "Components/XGlobal.h"

#include "../IRayDetector/QtLogger.h"

int main(int argc, char* argv[])
{
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
	QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
	QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
	QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::RoundPreferFloor);
#else
	qputenv("QT_SCALE_FACTOR", "1.5");
#endif
#endif

	QApplication a(argc, argv);
	a.setApplicationName("RayimDR");
	a.setWindowIcon(QPixmap(":/Resource/Image/logo_32x32.ico"));
	eApp->init();

	QtLogger::initialize();

	qDebug() << "程序运行，当前时间：" << QDateTime::currentDateTime();

	MainWindow w;
	w.setGeometry(QApplication::screens().first()->availableGeometry());
	w.showMaximized();

	return a.exec();
}