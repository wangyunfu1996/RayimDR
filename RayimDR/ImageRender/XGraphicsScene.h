#pragma once

#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QGraphicsLineItem>

/**
 * @brief 自定义图形场景类，用于管理和显示医学图像
 *
 * 负责管理以下图元：
 * - 主图像显示 (QGraphicsPixmapItem)
 * - ROI选择区域 (QGraphicsRectItem)
 * - 有效区域框 (QGraphicsRectItem)
 * - 中心线 (QGraphicsLineItem)
 */
class XGraphicsScene : public QGraphicsScene
{
    Q_OBJECT

public:
    XGraphicsScene(QObject* parent = nullptr);
    ~XGraphicsScene();

    // 图元访问接口
    QGraphicsPixmapItem* getPixmapItem() const { return m_pixmapItem; }

    QGraphicsRectItem* getRoiRectItem() const { return m_roiRectItem; }

    /**
     * @brief 设置显示的图像
     * @param pixmap 要显示的图像
     */
    void setPixmap(const QPixmap& pixmap);

    /**
     * @brief 更新图像显示，应用窗宽窗位调整
     * @param srcImage 原始16位灰度图像 (Format_Grayscale16)
     * @param windowWidth 窗宽值
     * @param windowLevel 窗位值
     */
    void updatePixmapDisplay(const QImage& srcImage, int windowWidth, int windowLevel);

    // ROI相关
    void setROIRect(const QRectF& rect);  ///< 设置ROI区域
    void clearROIRect();                  ///< 清除ROI区域
    void setROIVisible(bool visible);     ///< 设置ROI显示/隐藏

    // 有效区域显示
    void setValidRectVisible(bool visible);  ///< 设置有效区域框显示/隐藏

    // 中心线显示
    void setCenterLinesVisible(bool visible);  ///< 设置中心线显示/隐藏

    /**
     * @brief 更新所有辅助图元的显示状态
     * 根据当前图像大小重新计算各辅助图元的位置
     */
    void updateDisplay();

signals:
    void roiRectChanged(const QRectF& rect);  ///< ROI区域变化信号

private:
    void initItems();  ///< 初始化所有图元

private:
    // 图元对象
    QGraphicsPixmapItem* m_pixmapItem{nullptr};     ///< 主图像项
    QGraphicsRectItem* m_roiRectItem{nullptr};      ///< ROI选择区域
    QGraphicsRectItem* m_validRectItem{nullptr};    ///< 有效区域框
    QGraphicsLineItem* m_vCenterLineItem{nullptr};  ///< 垂直中心线
    QGraphicsLineItem* m_hCenterLineItem{nullptr};  ///< 水平中心线

    // 显示状态
    bool m_showValidRect{false};    ///< 是否显示有效区域框
    bool m_showCenterLines{false};  ///< 是否显示中心线
    bool m_roiVisible{false};       ///< 是否显示ROI区域
};
