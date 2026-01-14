#pragma once

#include <QGraphicsScene>

class XGraphicsScene : public QGraphicsScene
{
	Q_OBJECT

public:
	XGraphicsScene(QObject* parent = nullptr);
	~XGraphicsScene();

protected:
	// 重写鼠标事件
	void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
	void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
	void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

private:
	bool m_isDrawing;      // 标记是否正在绘制
	QPointF m_startPoint;  // 矩形起始点（场景坐标）
	QPointF m_endPoint;    // 矩形当前终点（场景坐标）
	QRectF m_currentRect;  // 当前矩形
};

