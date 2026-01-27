#include "XImageAdjustTool.h"

#include <qlayout.h>
#include <qdebug.h>

#include "ElaText.h"
#include "ElaSpinBox.h"
#include "ElaSlider.h"
#include "ElaCheckBox.h"

XImageAdjustTool::XImageAdjustTool(QWidget* parent) : QWidget(parent)
{
    initUI();
    initConnect();
}

XImageAdjustTool::~XImageAdjustTool() {}

void XImageAdjustTool::updateWLRange(int minValue, int maxValue)
{
    // 记录当前的值
    const int currentW = spinboxW->value();
    const int currentL = spinboxL->value();

    sliderW->blockSignals(true);
    sliderL->blockSignals(true);
    spinboxW->blockSignals(true);
    spinboxL->blockSignals(true);

    // TODO 优化 W 和 L 的最小最大区间
    int minW = minValue;
    int maxW = maxValue;
    int minL = minValue;
    int maxL = maxValue;

    sliderW->setMinimum(minW);
    sliderW->setMaximum(maxW);
    spinboxW->setMinimum(minW);
    spinboxW->setMaximum(maxW);

    sliderL->setMinimum(minL);
    sliderL->setMaximum(maxL);
    spinboxL->setMinimum(minL);
    spinboxL->setMaximum(maxL);

    sliderW->blockSignals(false);
    sliderL->blockSignals(false);
    spinboxW->blockSignals(false);
    spinboxL->blockSignals(false);

    // 如果 currentW 不在 minW - maxW 之间，修改 currentW
    if (currentW > maxW || currentW < minW)
    {
        spinboxW->setValue((minW + maxW) / 2);
    }

    // 如果 currentL 不在 minL - maxL 之间，修改 currentL
    if (currentL > maxL || currentL < minL)
    {
        spinboxL->setValue((minL + maxL) / 2);
    }
}

void XImageAdjustTool::updatePixelValueInfo(int x, int y, int value)
{
    pixelValueInfo->setText(QString("x: %1 y: %2 value: %3").arg(x).arg(y).arg(value));
}

void XImageAdjustTool::setWLValue(int W, int L)
{
    sliderW->setValue(W);
    sliderL->setValue(L);
}

void XImageAdjustTool::updateIdxRange(int imageListSize)
{
    sliderIdx->setMaximum(imageListSize - 1 > 0 ? imageListSize - 1 : 0);
}

void XImageAdjustTool::initUI()
{
    QHBoxLayout* hLayout = new QHBoxLayout;
    hLayout->setContentsMargins(0, 0, 0, 0);
    this->setLayout(hLayout);

    // 图像索引
    hLayout->addWidget(new ElaText("索引:", 14, this));
    sliderIdx = new ElaSlider(this);
    sliderIdx->setOrientation(Qt::Horizontal);
    sliderIdx->setMinimum(0);
    sliderIdx->setMaximum(0);
    hLayout->addWidget(sliderIdx);

    // 窗宽调整组件
    hLayout->addWidget(new ElaText("窗宽:", 14, this));
    sliderW = new ElaSlider(this);
    sliderW->setOrientation(Qt::Horizontal);
    sliderW->setMinimum(0);
    sliderW->setMaximum(65535);
    // sliderW->setEnabled(false);
    hLayout->addWidget(sliderW);

    spinboxW = new ElaSpinBox(this);
    spinboxW->setMinimumWidth(120);
    spinboxW->setButtonMode(ElaSpinBoxType::PMSide);
    spinboxW->setMinimum(0);
    spinboxW->setMaximum(65535);
    // spinboxW->setEnabled(false);
    hLayout->addWidget(spinboxW);

    hLayout->addSpacing(6);

    // 窗位调整组件
    hLayout->addWidget(new ElaText("窗位:", 14, this));
    sliderL = new ElaSlider(this);
    sliderL->setOrientation(Qt::Horizontal);
    sliderL->setMinimum(0);
    sliderL->setMaximum(65535);
    // sliderL->setEnabled(false);
    hLayout->addWidget(sliderL);

    spinboxL = new ElaSpinBox(this);
    spinboxL->setMinimumWidth(120);
    spinboxL->setButtonMode(ElaSpinBoxType::PMSide);
    spinboxL->setMinimum(0);
    spinboxL->setMaximum(65535);
    // spinboxL->setEnabled(false);
    hLayout->addWidget(spinboxL);

    hLayout->addSpacing(6);

    // ROI 选择
    checkboxROI = new ElaCheckBox("ROI调整窗宽窗位", this);
    checkboxROI->setVisible(false);
    hLayout->addWidget(checkboxROI);

    checkboxAutoWL = new ElaCheckBox("自动窗宽窗位调整", this);
    checkboxAutoWL->setVisible(false);
    hLayout->addWidget(checkboxAutoWL);

    hLayout->addSpacing(6);

    // 像素值显示
    pixelValueInfo = new ElaText(this);
    pixelValueInfo->setTextPixelSize(14);
    pixelValueInfo->setMinimumWidth(220);
    hLayout->addWidget(pixelValueInfo);
}

void XImageAdjustTool::initConnect()
{
    connect(sliderW, &QSlider::valueChanged, this, [this](int value) { spinboxW->setValue(value); });

    connect(sliderL, &QSlider::valueChanged, this, [this](int value) { spinboxL->setValue(value); });

    connect(spinboxW, QOverload<int>::of(&QSpinBox::valueChanged), this,
            [this](int value)
            {
                sliderW->blockSignals(true);
                sliderW->setValue(value);
                sliderW->blockSignals(false);

                qDebug() << "目标窗宽发生变化，W：" << spinboxW->value() << "，L：" << spinboxL->value();
                emit signalAdjustWL(spinboxW->value(), spinboxL->value());
            });

    connect(spinboxL, QOverload<int>::of(&QSpinBox::valueChanged), this,
            [this](int value)
            {
                sliderL->blockSignals(true);
                sliderL->setValue(value);
                sliderL->blockSignals(false);

                qDebug() << "目标窗位发生变化，W：" << spinboxW->value() << "，L：" << spinboxL->value();
                emit signalAdjustWL(spinboxW->value(), spinboxL->value());
            });

    connect(checkboxROI, &QCheckBox::toggled, this,
            [this](bool checked)
            {
                qDebug() << "设置允许ROI选择为：" << checked;
                if (checked && checkboxAutoWL->isChecked())
                {
                    checkboxAutoWL->setChecked(!checked);
                }
                emit signalSetROIEnable(checked);
            });

    connect(checkboxAutoWL, &QCheckBox::toggled, this,
            [this](bool checked)
            {
                qDebug() << "设置自动窗宽窗位调整为：" << checked;
                if (checked && checkboxROI->isChecked())
                {
                    checkboxROI->setChecked(!checked);
                }
                emit signalAutoWL(checked);
            });

    connect(sliderIdx, &QSlider::valueChanged, this,
            [this](int value)
            {
                qDebug() << "图像索引idx发生变化：" << value;
                emit signalImageIdxChanged(value);
            });
}
