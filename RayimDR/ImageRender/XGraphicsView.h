#pragma once

#include <QGraphicsView>

class XGraphicsScene;
class QGraphicsPixmapItem;
class QGraphicsRectItem;
class QGraphicsLineItem;

class XGraphicsView : public QGraphicsView
{
	Q_OBJECT

public:
	XGraphicsView(QWidget* parent = nullptr);
	~XGraphicsView();

	void updateDisplay();
	void updateImage(QImage image, bool adjustWL = false);
	void updateRoiRect(QRectF rect);
	void adjustWL(int width, int level);
	const QImage& getSrcU16Image() const;
	void clearROIRect();

public:
	void resetView();							// 重置视野
	void openImage();							// 打开文件
	void openImageFolder();						// 打开文件夹
	void saveImage();							// 另存为
	void setValidRectVisible(bool visible);		// 显示探测器有效视野
	void setCenterLinesVisible(bool visible);	// 绘制视野中心线
	void setROIEnable(bool enable);				// ROI调整窗宽窗位
	void setAutoWLEnable(bool enable);			// 自动调整窗宽窗位
	bool isAutoWL() const;
	void zoomIn();								// 放大
	void zoomOut();								// 缩小
	void showImage(int idx);
	void clearCurrentImageList();
	void addImageToList(QImage image);

private:
	void initContextMenu();

signals:
	void signalPixelHovered(int x, int y, int gray);
	void signalMinMaxValueChanged(int min, int max);
	void signalRoiWLChanged(int width, int level);
	void signalSrcU16ImageListSizeChanged(int n);

protected:
	void mouseMoveEvent(QMouseEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	void wheelEvent(QWheelEvent* event) override;

private:
	QImage currentSrcU16Image;
	QImage displayU8Image;

	QList<QImage> srcU16ImageList;

	bool autoWL = false;
	int W{ 0 };
	int L{ 0 };

	XGraphicsScene* xGraphicsScene{ nullptr };
	QGraphicsPixmapItem* pixmapItem{ nullptr };
	QGraphicsRectItem* roiRectItem{ nullptr };
	QGraphicsRectItem* validRectItem{ nullptr };
	QGraphicsLineItem* vCenterLineItem{ nullptr };
	QGraphicsLineItem* hCenterLineItem{ nullptr };

	bool enableROI{ false };
	bool roiSeleting{ false };
	bool showValidRect{ false };
	bool showCenterLines{ false };
};

