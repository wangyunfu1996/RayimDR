#pragma once

#include "ElaDialog.h"
#include "ui_DetCfgDialog.h"

class DetCfgDialog : public ElaDialog
{
	Q_OBJECT

public:
	DetCfgDialog(QWidget* parent = nullptr);
	~DetCfgDialog();

	void UpdateCorrectionOptions();
	void Active();
	void Deactive();
	void PowerOff();

private:
	Ui::DetCfgDialogClass ui;
};

