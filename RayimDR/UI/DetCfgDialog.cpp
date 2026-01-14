#include "DetCfgDialog.h"

#include "ElaUIHepler.h"

DetCfgDialog::DetCfgDialog(QWidget *parent)
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
}

DetCfgDialog::~DetCfgDialog()
{}

