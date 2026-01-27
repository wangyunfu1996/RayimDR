#pragma once

#include <QObject>

/**
 * @class XWindowLevelManager
 * @brief 窗宽窗位(Window Level)管理器
 *
 * 负责管理窗宽和窗位的计算、存储和通知
 * 降低View和Scene的耦合度
 */
class XWindowLevelManager : public QObject
{
    Q_OBJECT

public:
    XWindowLevelManager(QObject* parent = nullptr);
    ~XWindowLevelManager();

    // 获取当前窗宽和窗位
    int getWidth() const { return m_width; }

    int getLevel() const { return m_level; }

    // 直接设置窗宽窗位
    void setWindowLevel(int width, int level);

    // 根据最大最小值计算窗宽窗位
    void calculateFromMinMax(int minValue, int maxValue);

    // 根据ROI矩形和图像数据计算窗宽窗位
    void calculateFromROI(const QImage& image, const QRect& roiRect);

    // 重置窗宽窗位
    void reset();

signals:
    // 窗宽窗位发生变化时发出
    void windowLevelChanged(int width, int level);

private:
    int m_width{0};
    int m_level{0};
};
