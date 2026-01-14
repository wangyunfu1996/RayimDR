#pragma once

#include "ElaDialog.h"
#include "ui_XRayCfgDialog.h"

class XRayCfgDialog : public ElaDialog
{
	Q_OBJECT

public:
	XRayCfgDialog(QWidget* parent = nullptr);
	~XRayCfgDialog();

private:
	Ui::XRayCfgDialogClass ui;
};

