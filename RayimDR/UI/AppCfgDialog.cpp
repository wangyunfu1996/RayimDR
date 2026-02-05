#include "AppCfgDialog.h"

#include <qdebug.h>
#include <qbuttongroup.h>

#include "Components/XGlobal.h"

#include "ElaText.h"
#include "ElaUIHepler.h"

AppCfgDialog::AppCfgDialog(QWidget* parent) : ElaDialog(parent)
{
    ui.setupUi(this);

    this->setWindowTitle("设置软件参数");
    this->setWindowButtonFlag(ElaAppBarType::StayTopButtonHint, false);
    this->setWindowButtonFlag(ElaAppBarType::MinimizeButtonHint, false);
    this->setWindowButtonFlag(ElaAppBarType::MaximizeButtonHint, false);
    this->resize(640, 200);


    QButtonGroup* btnGroup = new QButtonGroup(this);
    btnGroup->addButton(ui.radioButton_stackedFrame);
    btnGroup->addButton(ui.radioButton_everyFrame);

    ElaUIHepler::ChangeToNormalStyle(this);
    ui.spinBox_detLowBattery->setRange(20, 100);

    connect(ui.pushButton_confirm, &QPushButton::clicked, this, [this]() { this->accept(); });
    connect(ui.pushButton_cancel, &QPushButton::clicked, this, [this]() { this->reject(); });

    connect(ui.radioButton_everyFrame, &QRadioButton::toggled, this,
            [this](bool checked) { xGlobal.SEND_SUBFRAME_FRAME_ON_ACQ = checked; });
}

AppCfgDialog::~AppCfgDialog() {}

int AppCfgDialog::getLowBatteryPercent()
{
    return ui.spinBox_detLowBattery->value();
}

bool AppCfgDialog::getAutoStartXrayOnAcq()
{
    return ui.checkBox_autoStartXRay->isChecked();
}

bool AppCfgDialog::getAutoStopXrayOnAcqStop()
{
    return ui.checkBox_autoStopXRay->isChecked();
}
