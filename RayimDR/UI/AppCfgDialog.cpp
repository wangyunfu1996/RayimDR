#include "AppCfgDialog.h"

#include <qdebug.h>

#include "Components/XGlobal.h"

AppCfgDialog::AppCfgDialog(QWidget* parent)
	: ElaDialog(parent)
{
	ui.setupUi(this);

	this->setWindowTitle("设置软件参数");
	this->setWindowButtonFlag(ElaAppBarType::StayTopButtonHint, false);
	this->setWindowButtonFlag(ElaAppBarType::MinimizeButtonHint, false);
	this->setWindowButtonFlag(ElaAppBarType::MaximizeButtonHint, false);
	this->resize(640, 200);

	connect(ui.pushButton_confirm, &QPushButton::clicked, this, [this]() {
		this->accept();
		});

	connect(ui.pushButton_cancel, &QPushButton::clicked, this, [this]() {
		this->reject();
		});

	ui.checkBox_cfg_before_acq->setChecked(xGlobal.CONFIG_BEFORE_ACQ);
	connect(ui.checkBox_cfg_before_acq, &QCheckBox::toggled, this, [this](bool checked) {
		xGlobal.CONFIG_BEFORE_ACQ = checked;
		});

}

AppCfgDialog::~AppCfgDialog()
{}

