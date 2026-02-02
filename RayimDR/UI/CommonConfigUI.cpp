#include "CommonConfigUI.h"

#include <QtConcurrent/QtConcurrent>

#include "ElaTheme.h"
#include "ElaDoubleSpinBox.h"

#include "Components/XSignalsHelper.h"
#include "ElaUIHepler.h"

#include "IRayDetector/NDT1717MA.h"
#include "VJXRAY/IXS120BP120P366.h"

CommonConfigUI::CommonConfigUI(QWidget* parent) : QWidget(parent)
{
    ui.setupUi(this);

    onThemeChanged(ElaThemeType::Dark);
    connect(eTheme, &ElaTheme::themeModeChanged, this, &CommonConfigUI::onThemeChanged);
    ElaUIHepler::ChangeToNormalStyle(this);

    for (int i(0); i < DET.GetMaxStackedNum(); i++)
    {
        ui.comboBox_stakcedNum->addItem(QString::number(i));
    }

    ui.lineEdit_ChargingStatus->setEnabled(false);
    ui.lineEdit_Battery_Remaining->setEnabled(false);

    initUIConnect();
}

CommonConfigUI::~CommonConfigUI() {}

bool CommonConfigUI::checkInputValid()
{
    QString errMsg;
    if (ui.spinBox_targetVoltage->value() > XRAY_MAX_VOLTAGE || ui.spinBox_targetVoltage->value() < XRAY_MIN_VOLTAGE)
    {
        errMsg = QString("设置的电压值必须在 %1kV - %2kV 之间").arg(XRAY_MIN_VOLTAGE).arg(XRAY_MAX_VOLTAGE);
        qDebug() << errMsg;
        emit xSignaHelper.signalShowErrorMessageBar(errMsg);
        return false;
    }

    if (ui.spinBox_targetCurrent->value() > XRAY_MAX_VOLTAGE || ui.spinBox_targetCurrent->value() < XRAY_MIN_VOLTAGE)
    {
        errMsg = QString("设置的电流值必须在 %1mA - %2mA 之间").arg(XRAY_MIN_CURRENT).arg(XRAY_MAX_CURRENT);
        qDebug() << errMsg;
        emit xSignaHelper.signalShowErrorMessageBar(errMsg);
        return false;
    }

    return true;
}

AcqCondition CommonConfigUI::getAcqCondition()
{
    AcqCondition acqCond;
    acqCond.mode = getModeFromUI();
    acqCond.frameRate = ui.comboBox_frameRate->currentText().toInt();
    acqCond.frameRate = acqCond.frameRate > 0 ? acqCond.frameRate : 1;
    acqCond.stackedFrame = ui.comboBox_stakcedNum->currentText().toInt();
    return acqCond;
}

void CommonConfigUI::onThemeChanged(ElaThemeType::ThemeMode themeMode)
{
    if (themeMode == ElaThemeType::Dark)
    {
        this->setStyleSheet(R"(
		QGroupBox {
			font-size: 15px;
			color: white;
		}
		QGroupBox::title {
			subcontrol-origin: margin; /* 以边距区域为定位基准 */
			subcontrol-position: top left; /* 位置在左上角 */
			font-weight: bold; /* 字体加粗 */
		}
		)");
    }
    else
    {
        this->setStyleSheet(R"(
		QGroupBox {
			font-size: 15px;
			color: black;
		}
		QGroupBox::title {
			subcontrol-origin: margin; /* 以边距区域为定位基准 */
			subcontrol-position: top left; /* 位置在左上角 */
			font-weight: bold; /* 字体加粗 */
		}
		)");
    }
}

void CommonConfigUI::initUIConnect()
{
    // 模式改变 帧率的最大值上线相应发生改变
    connect(ui.comboBox_mode, &QComboBox::currentTextChanged, this, &CommonConfigUI::changeMode);
    connect(&DET, &NDT1717MA::signalModeChanged, this,
            [this]()
            {
                auto status = DET.Status();
                updateUIFromMode(status.Mode);
            });

    connect(ui.comboBox_frameRate, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &CommonConfigUI::changeFrameRate);

    connect(&DET, &NDT1717MA::signalBatteryStatusChanged, this,
            [this]()
            {
                auto status = DET.Status();
                ui.lineEdit_ChargingStatus->setText(status.Battery_ChargingStatus == 1 ? "充电中" : "未充电");
                ui.lineEdit_Battery_Remaining->setText(QString::number(status.Battery_Remaining) + "%");
            });

    // 只在按下回车键时触发，不在失去焦点时触发
    // 通过 findChild 获取 QSpinBox 内部的 QLineEdit
    if (auto* currentLineEdit = ui.spinBox_targetCurrent->findChild<QLineEdit*>())
    {
        connect(currentLineEdit, &QLineEdit::returnPressed, this,
                [this]()
                {
                    QtConcurrent::run([this]()
                                      { IXS120BP120P366::Instance().setCurrent(ui.spinBox_targetCurrent->value()); });
                });
    }
    if (auto* voltageLineEdit = ui.spinBox_targetVoltage->findChild<QLineEdit*>())
    {
        connect(voltageLineEdit, &QLineEdit::returnPressed, this,
                [this]()
                {
                    QtConcurrent::run([this]()
                                      { IXS120BP120P366::Instance().setVoltage(ui.spinBox_targetVoltage->value()); });
                });
    }

    connect(&IXS120BP120P366::Instance(), &IXS120BP120P366::statusUpdated, this,
            [this](const XRaySourceStatus& status)
            {
                ui.lineEdit_currentCurrent->setText(QString::number(status.current / 1000.0, 'f', 2) + " mA");
                ui.lineEdit_currentVoltage->setText(QString::number(status.voltage, 'f', 2) + " kV");
                ui.lineEdit_currentPower->setText(QString::number((status.voltage * status.current) / 1000.0, 'f', 2) +
                                                  " W");
                ui.lineEdit_temperature->setText(QString::number(status.temperature, 'f', 1) + " ℃");
            });
}

void CommonConfigUI::changeMode(const QString& modeText)
{
    setUIEnable(false);
    std::string mode = getModeFromUI();
    // 移到后台线程执行
    auto future = QtConcurrent::run(
        [this, mode]()
        {
            bool bRet{true};

            if (!DET.Initialized())
            {
                emit xSignaHelper.signalShowErrorMessageBar("探测器未连接！");
                return false;
            }

            if (!DET.CanModifyCfg())
            {
                emit xSignaHelper.signalShowErrorMessageBar("当前不允许修改参数！");
                return false;
            }

            return DET.UpdateMode(mode);
        });

    auto* watcher = new QFutureWatcher<bool>(this);
    connect(watcher, &QFutureWatcher<bool>::finished, this,
            [this, watcher, mode]()
            {
                bool bRet = watcher->future().result();
                qDebug() << "修改探测器模式结束，执行结果：" << watcher->future().result();
                if (!bRet)
                {
                    emit xSignaHelper.signalShowErrorMessageBar("修改探测器模式失败！");
                }
                else
                {
                    emit xSignaHelper.signalShowSuccessMessageBar("修改探测器模式成功！");
                }
                watcher->deleteLater();

                updateUIFromMode(mode);
                setUIEnable(true);
            });
    watcher->setFuture(future);
    future.waitForFinished();
}

void CommonConfigUI::changeFrameRate(int nFrameRateComboboxIdx)
{
    int nFrameRate = ++nFrameRateComboboxIdx;
    int nSequenceIntervalTime = 1000 / nFrameRate;
    DET.UpdateSequenceIntervalTime(nSequenceIntervalTime);
}

std::string CommonConfigUI::getModeFromUI()
{
    if (ui.comboBox_mode->currentText().contains("1x1"))
        return "Mode5";
    else if (ui.comboBox_mode->currentText().contains("2x2"))
        return "Mode6";
    else if (ui.comboBox_mode->currentText().contains("3x3"))
        return "Mode7";
    else if (ui.comboBox_mode->currentText().contains("4x4"))
        return "Mode8";
    else
        return "Mode5";
}

void CommonConfigUI::setUIEnable(bool enable)
{
    ui.comboBox_mode->setEnabled(enable);
    ui.comboBox_frameRate->setEnabled(enable);
    ui.comboBox_stakcedNum->setEnabled(enable);
}

void CommonConfigUI::updateUIFromMode(std::string mode)
{
    qDebug() << "mode: " << mode.c_str();
    ui.comboBox_mode->blockSignals(true);
    ui.comboBox_frameRate->blockSignals(true);

    if (mode == "Mode5")
        ui.comboBox_mode->setCurrentText("1x1");
    else if (mode == "Mode6")
        ui.comboBox_mode->setCurrentText("2x2");
    else if (mode == "Mode7")
        ui.comboBox_mode->setCurrentText("3x3");
    else if (mode == "Mode8")
        ui.comboBox_mode->setCurrentText("4x4");

    ui.comboBox_frameRate->clear();
    int maxFrameRate = DET.GetModeMaxFrameRate(mode);
    for (int i(0); i < maxFrameRate; i++)
    {
        ui.comboBox_frameRate->addItem(QString::number(i + 1));
    }

    int frameRate = DET.GetFrameRate();
    qDebug() << "当前帧率: " << frameRate;
    ui.comboBox_frameRate->setCurrentText(QString::number(frameRate));

    ui.comboBox_mode->blockSignals(false);
    ui.comboBox_frameRate->blockSignals(false);
}
