#pragma once

#include <qdebug.h>

enum class AcqType
{
    DR = 0,
    CT
};

struct AcqCondition
{
    AcqType acqType{AcqType::DR};
    int voltage{40};   // kV
    int current{100};  // mA
    int frameRate{10};
    int frame{10};              // 帧数
    int stackedFrame{0};        // 叠加帧数
    std::string mode{"Mode5"};  // 1x1 2x2 3x3 4x4

    bool saveToFiles{false};
    QString savePath;
    QString saveType;
};

// 重载 QDebug 输出运算符
inline QDebug operator<<(QDebug debug, const AcqCondition& cond)
{
    // 将枚举转换为可读字符串
    QString acqTypeStr;
    switch (cond.acqType)
    {
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

    // 格式化输出
    QDebugStateSaver saver(debug);  // 保存debug状态，确保自动恢复格式
    debug.nospace() << "AcqCondition("
                    << "Type=" << acqTypeStr << ", Voltage=" << cond.voltage << "kV"
                    << ", Current=" << cond.current << "mA"
                    << ", FrameRate=" << cond.frameRate << "fps"
                    << ", Frames=" << cond.frame << ", StackedFrames=" << cond.stackedFrame
                    << ", DetMode=" << cond.mode.c_str() << ")";

    return debug;
}

const static int DET_WIDTH_1X1{4300};
const static int DET_HEIGHT_1X1{4300};
const static int DET_WIDTH_2X2{2133};
const static int DET_HEIGHT_2X2{2133};

const static int IMAGE_BUFFER_SIZE{10};

#define DET_TYPE_VIRTUAL 0
#define DET_TYPE_IRAY 1

#define DET_TYPE DET_TYPE_IRAY

#if DET_TYPE == DET_TYPE_VIRTUAL
#elif DET_TYPE == DET_TYPE_IRAY
#endif  // DET_TYPE

const static int XRAY_MIN_VOLTAGE{30};    // kV
const static int XRAY_MAX_VOLTAGE{120};   // kV
const static int XRAY_MIN_CURRENT{200};   // uA
const static int XRAY_MAX_CURRENT{1000};  // uA

const static std::string XRAY_DEVICE_IP = "192.168.10.1";
const static int XRAY_DEVICE_PORT = 10001;

const static std::string DET_HOST_IP_WIRED = "192.168.10.101";
const static std::string DET_HOST_IP_WIRELESS = "192.168.10.102";

// inline 变量确保在所有编译单元中共享同一个实例 (C++17)
inline bool AUTO_START_XRAY_ON_ACQ = false;
inline bool AUTO_STOP_XRAY_ON_ACQ_STOP = false;
