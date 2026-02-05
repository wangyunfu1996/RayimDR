#include "CommonConfigUI.h"

#include <QtConcurrent/QtConcurrent>

#include "ElaTheme.h"
#include "ElaDoubleSpinBox.h"

#include "Components/XSignalsHelper.h"
#include "ElaUIHepler.h"
#include "UI/XElaDialog.h"

#include "IRayDetector/NDT1717MA.h"
#include "VJXRAY/IXS120BP120P366.h"

#pragma warning(disable : 4305)

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

    ui.label_5->setVisible(false);
    ui.lineEdit_targetPower->setVisible(false);
    ui.label_6->setVisible(false);
    ui.lineEdit_currentPower->setVisible(false);

    ui.label_18->setVisible(false);
    ui.lineEdit_detErr->setVisible(false);

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

    if (ui.spinBox_targetCurrent->value() > XRAY_MAX_CURRENT || ui.spinBox_targetCurrent->value() < XRAY_MIN_CURRENT)
    {
        errMsg = QString("设置的电流值必须在 %1uA - %2uA 之间").arg(XRAY_MIN_CURRENT).arg(XRAY_MAX_CURRENT);
        qDebug() << errMsg;
        emit xSignaHelper.signalShowErrorMessageBar(errMsg);
        return false;
    }

    // 探测器点亮低于阈值则不允许进行采集
    if (!DET.CheckBatteryStateOK())
    {
        errMsg = "探测器电池电量过低，请先连接探测器电源或者充电后再进行采集！";
        emit xSignaHelper.signalShowErrorMessageBar(errMsg);
        return false;
    }

    // 如果射线源没有连接，不允许进行采集

    if (!IXS120BP120P366::Instance().isConnected())
    {
        errMsg = "射线源未连接，请检查连接后再进行采集！";
        emit xSignaHelper.signalShowErrorMessageBar(errMsg);
        return false;
    }

    // Check X-ray source status
    if (!IXS120BP120P366::Instance().xRayIsOn())
    {
        XElaDialog dialog("射线源未开启，是否先开启射线源？", XElaDialogType::ASK);
        if (AUTO_START_XRAY_ON_ACQ || dialog.showCentered() == QDialog::Accepted)
        {
            bool bRet = IXS120BP120P366::Instance().setVoltage(ui.spinBox_targetVoltage->value());
            bRet = IXS120BP120P366::Instance().setCurrent(ui.spinBox_targetCurrent->value());
            bRet = IXS120BP120P366::Instance().startXRay();
            int ptst = IXS120BP120P366::Instance().getPTST();
            QThread::msleep(1000 + ptst * 1000);
            return bRet;
        }
        else
        {
            return true;
        }
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

    connect(&DET, &NDT1717MA::signalStatusChanged, this,
            [this]()
            {
                auto status = DET.Status();
                ui.lineEdit_ChargingStatus->setText(status.Battery_ChargingStatus == 1 ? "充电中" : "未充电");
                ui.lineEdit_Battery_Remaining->setText(QString::number(status.Battery_Remaining) + "%");
                ui.lineEdit_connState->setText(status.Connected ? "已连接" : "未连接");
                if (status.State == 0)
                {
                    ui.lineEdit_detStat->setText("未知");
                }
                else if (status.State == 1)
                {
                    ui.lineEdit_detStat->setText("就绪");
                }
                else if (status.State == 2)
                {
                    ui.lineEdit_detStat->setText("忙碌");
                }
                else if (status.State == 3)
                {
                    ui.lineEdit_detStat->setText("休眠");
                }
            });

    connect(&DET, &NDT1717MA::signalErrorOccurred, this, [this](const QString& msg) { qDebug() << msg; });

    // 只在按下回车键时触发，不在失去焦点时触发
    // 通过 findChild 获取 QSpinBox 内部的 QLineEdit
    if (auto* currentLineEdit = ui.spinBox_targetCurrent->findChild<QLineEdit*>())
    {
        connect(currentLineEdit, &QLineEdit::returnPressed, this,
                [this]()
                {
                    auto future = QtConcurrent::run(
                        [this]() { IXS120BP120P366::Instance().setCurrent(ui.spinBox_targetCurrent->value()); });
                    auto* watcher = new QFutureWatcher<void>(this);
                    connect(watcher, &QFutureWatcher<void>::finished, this,
                            [this, watcher]() { watcher->deleteLater(); });
                });
    }

    if (auto* voltageLineEdit = ui.spinBox_targetVoltage->findChild<QLineEdit*>())
    {
        connect(voltageLineEdit, &QLineEdit::returnPressed, this,
                [this]()
                {
                    auto future = QtConcurrent::run(
                        [this]() { IXS120BP120P366::Instance().setVoltage(ui.spinBox_targetVoltage->value()); });
                    auto* watcher = new QFutureWatcher<void>(this);
                    connect(watcher, &QFutureWatcher<void>::finished, this,
                            [this, watcher]() { watcher->deleteLater(); });
                });
    }

    connect(&IXS120BP120P366::Instance(), &IXS120BP120P366::statusUpdated, this,
            [this](const XRaySourceStatus& status)
            {
                ui.lineEdit_currentCurrent->setText(QString::number(status.current * 1000, 'f', 3) + " uA");
                ui.lineEdit_currentVoltage->setText(QString::number(status.voltage, 'f', 2) + " kV");
                ui.lineEdit_currentPower->setText(QString::number((status.voltage * status.current), 'f', 2) + " W");
                ui.lineEdit_temperature->setText(QString::number(status.temperature, 'f', 1) + " ℃");

                // 根据 status.interlock 的状态设置 ui.lineEdit_interlock
                if (status.interlock == 0)
                {
                    ui.lineEdit_interlock->setText("正常");
                    ui.lineEdit_interlock->setStyleSheet("QLineEdit { color: green; }");
                }
                else
                {
                    ui.lineEdit_interlock->setText("互锁激活");
                    ui.lineEdit_interlock->setStyleSheet("QLineEdit { color: red; }");
                }
                ui.lineEdit_vdc->setText(QString::number(status.battery, 'f', 2) + " V");
            });

    connect(ui.pushButton_startXRay, &QPushButton::clicked, this,
            [this]()
            {
                IXS120BP120P366::Instance().setVoltage(ui.spinBox_targetVoltage->value());
                IXS120BP120P366::Instance().setCurrent(ui.spinBox_targetCurrent->value());
                IXS120BP120P366::Instance().startXRay();
            });

    connect(ui.pushButton_stopXRay, &QPushButton::clicked, this, [this]() { IXS120BP120P366::Instance().stopXRay(); });

    connect(ui.pushButton_clearErr, &QPushButton::clicked, this, [this]() { IXS120BP120P366::Instance().clearErr(); });

    connect(&IXS120BP120P366::Instance(), &IXS120BP120P366::xrayError, this,
            [this](const QString& errorMsg)
            {
                ui.lineEdit_errMsg->setText(errorMsg);
                emit xSignaHelper.signalShowErrorMessageBar("X射线源错误: " + errorMsg);
            });

    connect(&IXS120BP120P366::Instance(), &IXS120BP120P366::xrayErrorCleared, this,
            [this]() { ui.lineEdit_errMsg->clear(); });

    connect(ui.pushButton_startPreheat, &QPushButton::clicked, this,
            [this]()
            {
                ui.pushButton_startPreheat->setEnabled(false);
                ui.pushButton_startXRay->setEnabled(false);
                ui.pushButton_stopXRay->setEnabled(false);
                int waitTime{3};
                std::vector<int> voltages = {30, 40, 50, 60, 70, 80, 90, 100, 110, 120};
                std::vector<float> currents = {0.2000, 0.2889, 0.3778, 0.4667, 0.5556,
                                               0.6444, 0.7333, 0.8222, 0.9111, 1.0000};
                if (ui.comboBox_preheat->currentIndex() == 0)
                {
                    waitTime = 3;
                }
                else if (ui.comboBox_preheat->currentIndex() == 1)
                {
                    waitTime = 6;
                }
                else
                {
                    waitTime = 30;
                }

                auto future = QtConcurrent::run(
                    [this, waitTime, voltages, currents]()
                    {
                        IXS120BP120P366::Instance().setVoltage(voltages.at(0));
                        IXS120BP120P366::Instance().setCurrent(currents.at(0) * 1000);
                        IXS120BP120P366::Instance().startXRay();
                        int ptst = IXS120BP120P366::Instance().getPTST();
                        qDebug() << "ptst: " << ptst;
                        QThread::msleep(waitTime * 1000 + ptst * 1000);
                        for (int i(1); i < voltages.size(); i++)
                        {
                            IXS120BP120P366::Instance().setVoltage(voltages.at(i));
                            IXS120BP120P366::Instance().setCurrent(currents.at(i) * 1000);
                            QThread::msleep(waitTime * 1000);
                        }
                        IXS120BP120P366::Instance().stopXRay();
                    });

                auto* watcher = new QFutureWatcher<void>(this);
                connect(watcher, &QFutureWatcher<void>::finished, this,
                        [this, watcher]()
                        {
                            ui.pushButton_startPreheat->setEnabled(true);
                            ui.pushButton_startXRay->setEnabled(true);
                            ui.pushButton_stopXRay->setEnabled(true);
                            emit xSignaHelper.signalShowSuccessMessageBar("训管结束");
                        });
                watcher->setFuture(future);
            });

    connect(ui.pushButton_stopPreheat, &QPushButton::clicked, this,
            [this]()
            {
                IXS120BP120P366::Instance().getPTST();
                IXS120BP120P366::Instance().stopXRay();
                ui.pushButton_startPreheat->setEnabled(true);
                ui.pushButton_startXRay->setEnabled(true);
                ui.pushButton_stopXRay->setEnabled(true);
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
    // future.waitForFinished();
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
