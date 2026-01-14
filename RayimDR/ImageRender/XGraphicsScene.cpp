#include "XGraphicsScene.h"

#include <qpainter.h>
#include <qdebug.h>

#include <QGraphicsSceneMouseEvent>

XGraphicsScene::XGraphicsScene(QObject* parent)
	: QGraphicsScene(parent)
{
	initItems();
}

XGraphicsScene::~XGraphicsScene()
{}

void XGraphicsScene::initItems()
{
	// 创建主图像项
	m_pixmapItem = new QGraphicsPixmapItem();
	addItem(m_pixmapItem);
	
	// 创建ROI矩形（作为pixmapItem的子项）
	m_roiRectItem = new QGraphicsRectItem(m_pixmapItem);
	{
		// 设置半透明填充色 (RGBA中的A值控制透明度)
		QColor fillColor(128, 166, 233, 50); // 半透明的浅蓝色
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
		updateDisplay();
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
			emit roiRectChanged(rect);
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
	
	QRectF rect = m_pixmapItem->boundingRect();
	
	// 更新有效区域矩形
	if (m_validRectItem)
	{
		m_validRectItem->setRect(rect.adjusted(10, 10, -10, -10));
		m_validRectItem->setVisible(m_showValidRect);
	}
	
	// 更新中心线
	if (m_vCenterLineItem && m_hCenterLineItem)
	{
		m_vCenterLineItem->setLine(rect.left() + 10, rect.center().y(), 
									rect.right() - 10, rect.center().y());
		m_hCenterLineItem->setLine(rect.center().x(), rect.top() + 10, 
									rect.center().x(), rect.bottom() - 10);
		m_vCenterLineItem->setVisible(m_showCenterLines);
		m_hCenterLineItem->setVisible(m_showCenterLines);
	}
	
	update();
}

void XGraphicsScene::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
	// 可以在这里添加自定义的鼠标按下逻辑
	QGraphicsScene::mousePressEvent(event);
}

void XGraphicsScene::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
	// 可以在这里添加自定义的鼠标移动逻辑
	QGraphicsScene::mouseMoveEvent(event);
}

void XGraphicsScene::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
	// 可以在这里添加自定义的鼠标释放逻辑
	QGraphicsScene::mouseReleaseEvent(event);
}


