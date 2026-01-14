#pragma once

#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QGraphicsLineItem>

class XGraphicsScene : public QGraphicsScene
{
	Q_OBJECT

public:
	XGraphicsScene(QObject* parent = nullptr);
	~XGraphicsScene();

	// 图元访问接口
	QGraphicsPixmapItem* getPixmapItem() const { return m_pixmapItem; }
	QGraphicsRectItem* getRoiRectItem() const { return m_roiRectItem; }

	// 图像显示相关
	void setPixmap(const QPixmap& pixmap);
	void updatePixmapDisplay(const QImage& srcImage, int windowWidth, int windowLevel);

	// ROI相关
	void setROIRect(const QRectF& rect);
	void clearROIRect();
	void setROIVisible(bool visible);

	// 有效区域显示
	void setValidRectVisible(bool visible);

	// 中心线显示
	void setCenterLinesVisible(bool visible);

	// 更新所有图元的显示状态
	void updateDisplay();

signals:
	void roiRectChanged(const QRectF& rect);

protected:
	// 重写鼠标事件
	void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
	void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
	void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

private:
	void initItems(); // 初始化图元

private:
	// 图元对象
	QGraphicsPixmapItem* m_pixmapItem{ nullptr };
	QGraphicsRectItem* m_roiRectItem{ nullptr };
	QGraphicsRectItem* m_validRectItem{ nullptr };
	QGraphicsLineItem* m_vCenterLineItem{ nullptr };
	QGraphicsLineItem* m_hCenterLineItem{ nullptr };

	// 显示状态
	bool m_showValidRect{ false };
	bool m_showCenterLines{ false };
	bool m_roiVisible{ false };
};
