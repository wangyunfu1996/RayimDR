#include "XRayCfgDialog.h"

XRayCfgDialog::XRayCfgDialog(QWidget* parent)
	: ElaDialog(parent)
{
	ui.setupUi(this);
	this->setWindowTitle("射线源设置");
	this->setWindowButtonFlag(ElaAppBarType::StayTopButtonHint, false);
	this->setWindowButtonFlag(ElaAppBarType::MinimizeButtonHint, false);
	this->setWindowButtonFlag(ElaAppBarType::MaximizeButtonHint, false);
	this->resize(700, 200);

	connect(ui.pushButton_confirm, &QPushButton::clicked, this, [this]() {
		this->accept();
		});

	connect(ui.pushButton_cancel, &QPushButton::clicked, this, [this]() {
		this->reject();
		});

	ui.spinBox_targetVoltage->setButtonMode(ElaSpinBoxType::ButtonMode::PMSide);
	ui.spinBox_targetCurrent->setButtonMode(ElaSpinBoxType::ButtonMode::PMSide);

	ui.lineEdit_currentVoltage->setEnabled(false);
	ui.lineEdit_currentCurrent->setEnabled(false);
	ui.lineEdit_currentPower->setEnabled(false);
	ui.lineEdit_targetPower->setEnabled(false);
}

XRayCfgDialog::~XRayCfgDialog()
{}

