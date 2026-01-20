#include "DetCfgDialog.h"

#include "ElaUIHepler.h"

#include "../IRayDetector/NDT1717MA.h"

DetCfgDialog::DetCfgDialog(QWidget* parent)
	: ElaDialog(parent)
{
	ui.setupUi(this);

	this->setWindowTitle("探测器设置");
	this->setWindowButtonFlag(ElaAppBarType::StayTopButtonHint, false);
	this->setWindowButtonFlag(ElaAppBarType::MinimizeButtonHint, false);
	this->setWindowButtonFlag(ElaAppBarType::MaximizeButtonHint, false);

	this->resize(400, 200);

	connect(ui.pushButton_confirm, &QPushButton::clicked, this, [this]() {
		this->accept();
		});

	connect(ui.pushButton_cancel, &QPushButton::clicked, this, [this]() {
		this->reject();
		});

	ElaUIHepler::ChangeToNormalStyle(this);

	connect(ui.checkBox_Offset, &QCheckBox::toggled, this, &DetCfgDialog::UpdateCorrectionOptions);
	connect(ui.checkBox_Gain, &QCheckBox::toggled, this, &DetCfgDialog::UpdateCorrectionOptions);
	connect(ui.checkBox_Defect, &QCheckBox::toggled, this, &DetCfgDialog::UpdateCorrectionOptions);
}

DetCfgDialog::~DetCfgDialog()
{}

void DetCfgDialog::UpdateCorrectionOptions()
{
	int sw_offset = ui.checkBox_Offset->isChecked();
	int sw_gain = ui.checkBox_Gain->isChecked();
	int sw_defect = ui.checkBox_Defect->isChecked();
	DET.SetCorrectOption(sw_offset, sw_gain, sw_defect);
}

