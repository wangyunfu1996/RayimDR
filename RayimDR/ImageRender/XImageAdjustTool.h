#pragma once

#include <QWidget>

class ElaText;
class ElaSlider;
class ElaSpinBox;
class ElaCheckBox;

class XImageAdjustTool : public QWidget
{
    Q_OBJECT

public:
    XImageAdjustTool(QWidget* parent = nullptr);
    ~XImageAdjustTool();

    // minValue 和 maxValue 分别是一张灰度图像的像素最小值和最大值
    void updateWLRange(int minValue, int maxValue);
    void updatePixelValueInfo(int x, int y, int value);
    void setWLValue(int W, int L);
    void updateIdxRange(int imageListSize);

private:
    void initUI();
    void initConnect();

signals:
    // 只有 spinbox 的值发生变化 才会发出 signalAdjustWL
    void signalAdjustWL(int w, int l);
    void signalSetROIEnable(bool enable);
    void signalAutoWL(bool open);
    void signalImageIdxChanged(int idx);

private:
    ElaText* pixelValueInfo{nullptr};

    // 索引
    ElaSlider* sliderIdx{nullptr};

    // 窗宽
    ElaSlider* sliderW{nullptr};
    ElaSpinBox* spinboxW{nullptr};

    // 窗位
    ElaSlider* sliderL{nullptr};
    ElaSpinBox* spinboxL{nullptr};

    // 开启 ROI 选择
    ElaCheckBox* checkboxROI{nullptr};

    // 自动 WL
    ElaCheckBox* checkboxAutoWL{nullptr};
};
