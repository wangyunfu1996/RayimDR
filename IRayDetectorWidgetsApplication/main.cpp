#include <QtWidgets/QApplication>

#include "IRayDetectorWidgetsApplication.h"

#include "../IRayDetector/IRayDetector.h"
#include "../IRayDetector/QtLogger.h"

int main(int argc, char* argv[])
{
	QApplication app(argc, argv);
	QtLogger::initialize();

	//if (DET.Initialize() != 0)
	//{
	//	qDebug() << "探测器初始化失败！";
	//	DET.DeInitialize();
	//}
	//else
	//{
	//	DET.StartQueryStatus();
	//}

	IRayDetectorWidgetsApplication window;
	window.show();
	return app.exec();
}
