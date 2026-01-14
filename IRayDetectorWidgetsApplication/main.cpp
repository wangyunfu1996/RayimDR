#include <QtWidgets/QApplication>

#include "IRayDetectorWidgetsApplication.h"
#include "QtLogger.h"

#include "../IRayDetector/IRayDetector.h"

int main(int argc, char* argv[])
{
	QApplication app(argc, argv);
	QtLogger::initialize();

	if (DET.Initialize() != 0)
	{
		qDebug() << "探测器初始化失败！";
		DET.DeInitialize();
	}
	else
	{
		DET.UpdateMode("Mode5");
		int sw_offset{ -1 };
		int sw_gain{ -1 };
		int sw_defect{ -1 };
		DET.GetCurrentCorrectOption(sw_offset, sw_gain, sw_defect);

		sw_offset = 1;
		sw_gain = 1;
		sw_defect = 1;
		DET.SetCorrectOption(sw_offset, sw_gain, sw_defect);

		DET.SetPreviewImageEnable(0);
	}

	IRayDetectorWidgetsApplication window;
	window.show();
	return app.exec();
}
