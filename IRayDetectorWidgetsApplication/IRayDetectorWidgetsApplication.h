#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_IRayDetectorWidgetsApplication.h"

class IRayDetectorWidgetsApplication : public QMainWindow
{
	Q_OBJECT

public:
	IRayDetectorWidgetsApplication(QWidget* parent = nullptr);
	~IRayDetectorWidgetsApplication();

private:
	Ui::IRayDetectorWidgetsApplicationClass ui;
};

