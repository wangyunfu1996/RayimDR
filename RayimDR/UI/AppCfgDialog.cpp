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
    ui.spinBox_detLowBattery->setValue(xGlobal.DET_LOW_BATTERY_THRESHOLD);
    ui.checkBox_autoStartXRay->setChecked(xGlobal.AUTO_START_XRAY_ON_ACQ);
    ui.checkBox_autoStopXRay->setChecked(xGlobal.AUTO_STOP_XRAY_ON_ACQ_STOP);
    ui.radioButton_everyFrame->setChecked(xGlobal.SEND_SUBFRAME_FRAME_ON_ACQ);
    ui.radioButton_stackedFrame->setChecked(!xGlobal.SEND_SUBFRAME_FRAME_ON_ACQ);
    ui.checkBox_FlipHorizontal->setChecked(xGlobal.FLIP_HORIZONTAL);
    ui.checkBox_FlipVertical->setChecked(xGlobal.FLIP_VERTICAL);

    connect(ui.pushButton_confirm, &QPushButton::clicked, this,
            [this]()
            {
                qDebug() << "DET_LOW_BATTERY_THRESHOLD: " << xGlobal.DET_LOW_BATTERY_THRESHOLD
                         << " AUTO_START_XRAY_ON_ACQ: " << xGlobal.AUTO_START_XRAY_ON_ACQ
                         << " AUTO_STOP_XRAY_ON_ACQ_STOP: " << xGlobal.AUTO_STOP_XRAY_ON_ACQ_STOP
                         << " SEND_SUBFRAME_FRAME_ON_ACQ: " << xGlobal.SEND_SUBFRAME_FRAME_ON_ACQ;

                this->accept();
            });
    connect(ui.pushButton_cancel, &QPushButton::clicked, this, [this]() { this->reject(); });

    connect(ui.spinBox_detLowBattery, qOverload<int>(&QSpinBox::valueChanged), this,
            [this](int value) { xGlobal.DET_LOW_BATTERY_THRESHOLD = value; });
    connect(ui.checkBox_autoStartXRay, &QCheckBox::toggled, this,
            [this]() { xGlobal.AUTO_START_XRAY_ON_ACQ = ui.checkBox_autoStartXRay->isChecked(); });
    connect(ui.checkBox_autoStopXRay, &QCheckBox::toggled, this,
            [this]() { xGlobal.AUTO_STOP_XRAY_ON_ACQ_STOP = ui.checkBox_autoStopXRay->isChecked(); });
    connect(ui.radioButton_everyFrame, &QRadioButton::toggled, this,
            [this](bool checked) { xGlobal.SEND_SUBFRAME_FRAME_ON_ACQ = checked; });
    connect(ui.checkBox_FlipHorizontal, &QCheckBox::toggled, this,
            [this]() { xGlobal.FLIP_HORIZONTAL = ui.checkBox_FlipHorizontal->isChecked(); });
    connect(ui.checkBox_FlipVertical, &QCheckBox::toggled, this,
            [this]() { xGlobal.FLIP_VERTICAL = ui.checkBox_FlipVertical->isChecked(); });
}

AppCfgDialog::~AppCfgDialog() {}
