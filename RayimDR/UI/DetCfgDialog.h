#pragma once

#include "ElaDialog.h"
#include "ui_DetCfgDialog.h"

class DetCfgDialog : public ElaDialog
{
	Q_OBJECT

public:
	DetCfgDialog(QWidget* parent = nullptr);
	~DetCfgDialog();

private:
	Ui::DetCfgDialogClass ui;
};

