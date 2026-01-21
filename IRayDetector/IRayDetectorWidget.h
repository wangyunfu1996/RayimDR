#pragma once

#include "iraydetector_global.h"
#include <QtWidgets/QMainWindow>

namespace Ui
{
	class IRayDetectorWidgetClass;
}

class IRAYDETECTOR_EXPORT IRayDetectorWidget : public QMainWindow
{
	Q_OBJECT

public:
	IRayDetectorWidget(QWidget* parent = nullptr);
	~IRayDetectorWidget();

private:
	Ui::IRayDetectorWidgetClass* ui{ nullptr };
};

