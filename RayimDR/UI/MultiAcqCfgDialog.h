#pragma once

#include "ElaDialog.h"
#include "ui_MultiAcqCfgDialog.h"

class MultiAcqCfgDialog : public ElaDialog
{
	Q_OBJECT

public:
	MultiAcqCfgDialog(QWidget* parent = nullptr);
	~MultiAcqCfgDialog();

	bool getCfg(int& n, 
		bool& saveToFiles, 
		QString& savePath,
		QString& saveType);

private:
	Ui::MultiAcqCfgDialogClass ui;

	const int nMin{ 1 };
	const int nMax{ 1000 };
};

