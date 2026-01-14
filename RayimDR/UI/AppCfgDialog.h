#pragma once

#include "ElaDialog.h"
#include "ui_AppCfgDialog.h"

class AppCfgDialog : public ElaDialog
{
	Q_OBJECT

public:
	AppCfgDialog(QWidget* parent = nullptr);
	~AppCfgDialog();

private:
	Ui::AppCfgDialogClass ui;
};

