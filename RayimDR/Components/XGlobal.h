#pragma once

#include <qdebug.h>

#define DET_TYPE_VIRTUAL 0
#define DET_TYPE_IRAY 1

#define DET_TYPE DET_TYPE_IRAY

#if DET_TYPE == DET_TYPE_VIRTUAL
#elif DET_TYPE == DET_TYPE_IRAY
#endif  // DET_TYPE

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

    // 图像翻转和镜像选项
    bool enableFlipHorizontal{true};  // 水平翻转（镜像）
    bool enableFlipVertical{false};   // 垂直翻转（上下翻转）
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

#define xGlobal XGlobal::Instance()

// XGlobal 单例类 - 管理全局配置参数
class XGlobal
{
public:
    // 获取单例实例（线程安全，C++11）
    static XGlobal& Instance()
    {
        static XGlobal instance;
        return instance;
    }

    // 删除拷贝构造和赋值操作
    XGlobal(const XGlobal&) = delete;
    XGlobal& operator=(const XGlobal&) = delete;

    int DET_WIDTH_1X1{4300};
    int DET_HEIGHT_1X1{4300};
    int IMAGE_BUFFER_SIZE{10};


    int XRAY_MIN_VOLTAGE{30};    // kV
    int XRAY_MAX_VOLTAGE{120};   // kV
    int XRAY_MIN_CURRENT{200};   // uA
    int XRAY_MAX_CURRENT{1000};  // uA

    std::string XRAY_DEVICE_IP = "192.168.10.1";
    int XRAY_DEVICE_PORT = 10001;

    std::string DET_HOST_IP_WIRED = "192.168.10.101";
    std::string DET_HOST_IP_WIRELESS = "192.168.10.102";

    // 采集时自动启动X射线
    bool AUTO_START_XRAY_ON_ACQ = true;
    // 采集停止时自动关闭X射线
    bool AUTO_STOP_XRAY_ON_ACQ_STOP = true;

    // 采集时发送子帧数据
    bool SEND_SUBFRAME_FRAME_ON_ACQ = true;

private:
    // 私有构造函数
    XGlobal() = default;
    ~XGlobal() = default;
};
