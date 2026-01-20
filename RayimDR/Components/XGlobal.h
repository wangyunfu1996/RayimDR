#pragma once

#include <qdebug.h>

enum class AcqType
{
	DR = 0,
	CT
};

struct AcqCondition
{
	AcqType acqType{ AcqType::DR };
	int voltage{ 40 };			// kV
	int current{ 100 };			// mA
	int frameRate{ 10 };
	int frame{ 10 };			// 帧数 可以理解为角度数
	int stackedFrame{ 0 };		// 叠加帧数
	int detMode{ 1 };			// 1x1 2x2 3x3 4x4

	bool saveToFiles{ false };
	QString savePath;
	QString saveType;
};

// 重载 QDebug 输出运算符
inline QDebug operator<<(QDebug debug, const AcqCondition& cond)
{
	// 将枚举转换为可读字符串
	QString acqTypeStr;
	switch (cond.acqType) {
	case AcqType::DR:
		acqTypeStr = "DR";
		break;
	case AcqType::CT:
		acqTypeStr = "CT";
		break;
	default:
		acqTypeStr = "Other";
		break;
	}

	// 将探测器模式转换为可读格式
	QString detModeStr = QString("%1x%1").arg(cond.detMode);

	// 格式化输出
	QDebugStateSaver saver(debug);  // 保存debug状态，确保自动恢复格式
	debug.nospace() << "AcqCondition("
		<< "Type=" << acqTypeStr
		<< ", Voltage=" << cond.voltage << "kV"
		<< ", Current=" << cond.current << "mA"
		<< ", FrameRate=" << cond.frameRate << "fps"
		<< ", Frames=" << cond.frame
		<< ", StackedFrames=" << cond.stackedFrame
		<< ", DetMode=" << detModeStr
		<< ")";

	return debug;
}

const static int DET_WIDTH{ 4300 };
const static int DET_HEIGHT{ 4300 };

const static int IMAGE_BUFFER_SIZE{ 10 };

#define DET_TYPE_VIRTUAL 0
#define DET_TYPE_IRAY 1

#define DET_TYPE DET_TYPE_IRAY

#if DET_TYPE == DET_TYPE_VIRTUAL
#elif DET_TYPE == DET_TYPE_IRAY
#endif // DET_TYPE


#define xGlobal XGlobal::Instance()

class XGlobal
{
private:
	XGlobal()
	{

	}

	~XGlobal()
	{

	}

public:
	static XGlobal& Instance()
	{
		static XGlobal instance;
		return instance;
	}

public:
	bool CONFIG_BEFORE_ACQ{ false };

	int VOLTAGE_MAX{ 160 };
	int VOLTAGE_MIN{ 40 };

	int CURRENT_MAX{ 1000 };
	int CURRENT_MIN{ 10 };

	int POWER_MAX{ 0 };
	int POWER_MIN{ 50 };

	int SPEED_MAX{ 10 };
	int SPEED_MIN{ -10 };

	std::atomic<int> MAX_FPS{ 1 };
};

