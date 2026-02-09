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

    bool init();

    QString getString(const QString& section, const QString& key, const QString& defaultValue = QString()) const;
    int getInt(const QString& section, const QString& key, int defaultValue = 0) const;
    double getDouble(const QString& section, const QString& key, double defaultValue = 0.0) const;
    bool getBool(const QString& section, const QString& key, bool defaultValue = false) const;

    void setString(const QString& section, const QString& key, const QString& value);
    void setInt(const QString& section, const QString& key, int value);
    void setDouble(const QString& section, const QString& key, double value);
    void setBool(const QString& section, const QString& key, bool value);

    bool save();

    // 删除拷贝构造和赋值操作
    XGlobal(const XGlobal&) = delete;
    XGlobal& operator=(const XGlobal&) = delete;

private:
    // 私有构造函数
    XGlobal();
    ~XGlobal();

    class IniReader* m_IniReader{nullptr};
};
