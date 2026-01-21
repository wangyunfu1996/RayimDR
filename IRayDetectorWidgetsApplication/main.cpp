#include <QtWidgets/QApplication>

#include "../IRayDetector/IRayDetectorWidget.h"
#include "../IRayDetector/NDT1717MA.h"
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

	IRayDetectorWidget window;
	window.show();
	return app.exec();
}
