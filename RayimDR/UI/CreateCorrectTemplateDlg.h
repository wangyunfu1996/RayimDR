#pragma once

#include "ElaDialog.h"
#include "ui_CreateCorrectTemplateDlg.h"

class CreateCorrectTemplateDlg : public ElaDialog
{
	Q_OBJECT

public:
	CreateCorrectTemplateDlg(QWidget* parent = nullptr);
	~CreateCorrectTemplateDlg();

private:

	void Abort();

	bool OffsetGeneration();

	bool GainInit();
	bool GainStartAcq();
	bool GainSelectAll();
	bool GainGeneration();

private:
	Ui::CreateCorrectTemplateDlgClass ui;
};

