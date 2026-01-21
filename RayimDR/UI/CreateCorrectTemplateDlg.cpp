#include "CreateCorrectTemplateDlg.h"

#include "../IRayDetector/NDT1717MA.h"

CreateCorrectTemplateDlg::CreateCorrectTemplateDlg(QWidget* parent)
	: ElaDialog(parent)
{
	ui.setupUi(this);

	connect(ui.pushButton_OffsetGenerationStart, &QPushButton::clicked, this, &CreateCorrectTemplateDlg::OffsetGeneration);
	connect(ui.pushButton_OffsetGenerationAbort, &QPushButton::clicked, this, &CreateCorrectTemplateDlg::Abort);

	connect(ui.pushButton_GainInit, &QPushButton::clicked, this, &CreateCorrectTemplateDlg::GainInit);
	connect(ui.pushButton_GainAbort, &QPushButton::clicked, this, &CreateCorrectTemplateDlg::Abort);
	connect(ui.pushButton_GainStartAcq, &QPushButton::clicked, this, &CreateCorrectTemplateDlg::GainStartAcq);
	connect(ui.pushButton_GainSelectAll, &QPushButton::clicked, this, &CreateCorrectTemplateDlg::GainSelectAll);
	connect(ui.pushButton_GainGeneration, &QPushButton::clicked, this, &CreateCorrectTemplateDlg::GainGeneration);


	connect(&DET, &NDT1717MA::signalGainImageReceived, this, [this](int nCenterValue) {
		ui.lineEdit_GainCenterValue->setText(QString::number(nCenterValue));
		});
}

CreateCorrectTemplateDlg::~CreateCorrectTemplateDlg()
{}

void CreateCorrectTemplateDlg::Abort()
{
	DET.Abort();
}

bool CreateCorrectTemplateDlg::OffsetGeneration()
{
	return 0 == DET.OffsetGeneration();
}

bool CreateCorrectTemplateDlg::GainInit()
{
	return 0 == DET.GainInit();
}

bool CreateCorrectTemplateDlg::GainStartAcq()
{
	return DET.GainStartAcq();
}

bool CreateCorrectTemplateDlg::GainSelectAll()
{
	return DET.GainSelectAll();
}

bool CreateCorrectTemplateDlg::GainGeneration()
{
	return DET.GainGeneration();
}

