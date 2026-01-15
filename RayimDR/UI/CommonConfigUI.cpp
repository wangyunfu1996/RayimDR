#include "CommonConfigUI.h"

#include "ElaTheme.h"
#include "ElaDoubleSpinBox.h"

#include "Components/XSignalsHelper.h"
#include "ElaUIHepler.h"

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
	connect(ui.comboBox_mode, &QComboBox::currentTextChanged, this, [this](const QString& text) {
		if ("1x1" == text)
		{
			xGlobal.MAX_FPS.store(1);
			ui.spinBox_fps->setMaximum(1);
		}
		else if ("2x2" == text)
		{
			xGlobal.MAX_FPS.store(4);
			ui.spinBox_fps->setMaximum(4);
		}
		else if ("3x3" == text)
		{
			xGlobal.MAX_FPS.store(10);
			ui.spinBox_fps->setMaximum(10);
		}
		else if ("4x4" == text)
		{
			xGlobal.MAX_FPS.store(16);
			ui.spinBox_fps->setMaximum(16);
		}
		});
}

