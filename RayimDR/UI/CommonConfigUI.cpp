#include "CommonConfigUI.h"

#include <QtConcurrent/QtConcurrent>

#include "ElaTheme.h"
#include "ElaDoubleSpinBox.h"

#include "Components/XSignalsHelper.h"
#include "ElaUIHepler.h"

#if DET_TYPE == DET_TYPE_VIRTUAL
#elif DET_TYPE == DET_TYPE_IRAY
#include "../IRayDetector/NDT1717MA.h"
#endif // DET_TYPE

CommonConfigUI::CommonConfigUI(QWidget* parent)
	: QWidget(parent)
{
	ui.setupUi(this);

	onThemeChanged(ElaThemeType::Dark);
	connect(eTheme, &ElaTheme::themeModeChanged, this, &CommonConfigUI::onThemeChanged);
	ElaUIHepler::ChangeToNormalStyle(this);

	initUIConnect();
}

CommonConfigUI::~CommonConfigUI()
{}

bool CommonConfigUI::checkInputValid()
{
	QString errMsg;
	if (ui.spinBox_targetVoltage->value() > xGlobal.VOLTAGE_MAX ||
		ui.spinBox_targetVoltage->value() < xGlobal.VOLTAGE_MIN)
	{
		errMsg = QString("设置的电压值必须在 %1kV - %2kV 之间").arg(xGlobal.VOLTAGE_MIN).arg(xGlobal.VOLTAGE_MAX);
		qDebug() << errMsg;
		emit xSignaHelper.signalShowErrorMessageBar(errMsg);
		return false;
	}

	if (ui.spinBox_targetCurrent->value() > xGlobal.CURRENT_MAX ||
		ui.spinBox_targetCurrent->value() < xGlobal.CURRENT_MIN)
	{
		errMsg = QString("设置的电流值必须在 %1mA - %2mA 之间").arg(xGlobal.CURRENT_MIN).arg(xGlobal.CURRENT_MAX);
		qDebug() << errMsg;
		emit xSignaHelper.signalShowErrorMessageBar(errMsg);
		return false;
	}

	return true;
}

AcqCondition CommonConfigUI::getAcqCondition()
{
	AcqCondition acqCond;
	acqCond.detMode = ui.comboBox_mode->currentIndex() + 1;
	acqCond.frameRate = ui.spinBox_fps->value();
	acqCond.stackedFrame = ui.spinBox_add->value();
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
}

void CommonConfigUI::changeMode(const QString& modeText)
{
	setUIEnable(false);

	std::string mode;
	if (modeText.contains("1x1"))
	{
		xGlobal.MAX_FPS.store(1);
		ui.spinBox_fps->setMaximum(1);
		mode = "Mode5";
	}
	else if (modeText.contains("2x2"))
	{
		xGlobal.MAX_FPS.store(4);
		ui.spinBox_fps->setMaximum(4);
		mode = "Mode6";
	}
	else if (modeText.contains("3x3"))
	{
		xGlobal.MAX_FPS.store(10);
		ui.spinBox_fps->setMaximum(10);
		mode = "Mode7";
	}
	else if (modeText.contains("4x4"))
	{
		xGlobal.MAX_FPS.store(16);
		ui.spinBox_fps->setMaximum(16);
		mode = "Mode8";
	}

	// 移到后台线程执行
	auto future = QtConcurrent::run([this, mode]() {
		bool bRet{ true };

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
	connect(watcher, &QFutureWatcher<bool>::finished, this, [this, watcher]() {
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
		setUIEnable(true);
		});
	watcher->setFuture(future);

}

void CommonConfigUI::setUIEnable(bool enable)
{
	ui.comboBox_mode->setEnabled(enable);
	ui.spinBox_fps->setEnabled(enable);
	ui.spinBox_add->setEnabled(enable);
}

