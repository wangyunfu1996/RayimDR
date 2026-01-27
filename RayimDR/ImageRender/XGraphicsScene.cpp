#include "XGraphicsScene.h"
#include "XImageHelper.h"

#include <qpainter.h>
#include <qdebug.h>

#include <QGraphicsSceneMouseEvent>

XGraphicsScene::XGraphicsScene(QObject* parent) : QGraphicsScene(parent)
{
    initItems();
}

XGraphicsScene::~XGraphicsScene() {}

void XGraphicsScene::initItems()
{
    // 创建主图像项
    m_pixmapItem = new QGraphicsPixmapItem();
    addItem(m_pixmapItem);

    // 创建ROI矩形（作为pixmapItem的子项）
    m_roiRectItem = new QGraphicsRectItem(m_pixmapItem);
    {
        // 设置半透明填充色 (RGBA中的A值控制透明度)
        QColor fillColor(128, 166, 233, 50);  // 半透明的浅蓝色
        QBrush brush(fillColor);
        m_roiRectItem->setBrush(brush);

        // 设置边框
        QPen pen(QColor("#3C7FB1"), 1, Qt::SolidLine);
        m_roiRectItem->setPen(pen);
        m_roiRectItem->setVisible(false);
    }

    // 创建有效区域矩形
    m_validRectItem = new QGraphicsRectItem(m_pixmapItem);
    {
        QPen pen(Qt::blue, 1, Qt::DashLine);
        m_validRectItem->setPen(pen);
        m_validRectItem->setBrush(Qt::NoBrush);
        m_validRectItem->setVisible(false);
    }

    // 创建中心线
    m_vCenterLineItem = new QGraphicsLineItem(m_pixmapItem);
    m_hCenterLineItem = new QGraphicsLineItem(m_pixmapItem);
    {
        QPen pen(Qt::blue, 1, Qt::DashLine);
        m_vCenterLineItem->setPen(pen);
        m_hCenterLineItem->setPen(pen);
        m_vCenterLineItem->setVisible(false);
        m_hCenterLineItem->setVisible(false);
    }
}

void XGraphicsScene::setPixmap(const QPixmap& pixmap)
{
    if (m_pixmapItem)
    {
        m_pixmapItem->setPixmap(pixmap);

        // 更新场景矩形以匹配新图像大小
        if (!pixmap.isNull())
        {
            // 设置场景矩形为 pixmapItem 的边界矩形
            setSceneRect(m_pixmapItem->boundingRect());
        }

        updateDisplay();
    }
}

void XGraphicsScene::updatePixmapDisplay(const QImage& srcImage, int windowWidth, int windowLevel)
{
    if (srcImage.isNull() || srcImage.format() != QImage::Format_Grayscale16)
    {
        return;
    }

    // 使用窗宽窗位调整图像并转换为QPixmap
    // 注意：adjustWL返回Format_Grayscale8格式，适合显示
    QImage displayImage = XImageHelper::adjustWL(srcImage, windowWidth, windowLevel);

    if (!displayImage.isNull())
    {
        setPixmap(QPixmap::fromImage(displayImage));
    }
}

void XGraphicsScene::setROIRect(const QRectF& rect)
{
    if (m_roiRectItem)
    {
        // 只有当矩形真正改变时才更新和发出信号
        if (m_roiRectItem->rect() != rect)
        {
            m_roiRectItem->setRect(rect);

            // 只有在矩形有效时才发出信号
            if (!rect.isEmpty() && rect.isValid())
            {
                emit roiRectChanged(rect);
            }
        }
    }
}

void XGraphicsScene::clearROIRect()
{
    setROIRect(QRectF());
}

void XGraphicsScene::setROIVisible(bool visible)
{
    m_roiVisible = visible;
    if (m_roiRectItem)
    {
        m_roiRectItem->setVisible(visible);
    }
}

void XGraphicsScene::setValidRectVisible(bool visible)
{
    m_showValidRect = visible;
    updateDisplay();
}

void XGraphicsScene::setCenterLinesVisible(bool visible)
{
    m_showCenterLines = visible;
    updateDisplay();
}

void XGraphicsScene::updateDisplay()
{
    if (!m_pixmapItem || m_pixmapItem->pixmap().isNull())
    {
        return;
    }

    const QRectF rect = m_pixmapItem->boundingRect();
    const qreal margin = 10.0;

    // 更新有效区域矩形
    if (m_validRectItem)
    {
        m_validRectItem->setRect(rect.adjusted(margin, margin, -margin, -margin));
        m_validRectItem->setVisible(m_showValidRect);
    }

    // 更新中心线
    if (m_vCenterLineItem && m_hCenterLineItem)
    {
        const QPointF center = rect.center();

        // 水平中心线
        m_hCenterLineItem->setLine(rect.left() + margin, center.y(), rect.right() - margin, center.y());
        // 垂直中心线
        m_vCenterLineItem->setLine(center.x(), rect.top() + margin, center.x(), rect.bottom() - margin);

        m_hCenterLineItem->setVisible(m_showCenterLines);
        m_vCenterLineItem->setVisible(m_showCenterLines);
    }
}
