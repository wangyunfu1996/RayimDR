#pragma once

#include "ElaDialog.h"
#include "ui_MotionCfgDialog.h"

class MotionCfgDialog : public ElaDialog
{
	Q_OBJECT

public:
	MotionCfgDialog(QWidget *parent = nullptr);
	~MotionCfgDialog();

private:
	Ui::MotionCfgDialogClass ui;
};

