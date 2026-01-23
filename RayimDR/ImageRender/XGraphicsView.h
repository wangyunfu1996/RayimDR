#pragma once

#include <QGraphicsView>

class XGraphicsScene;
class XWindowLevelManager;

/**
 * @brief 自定义图形视图类，用于显示和交互医学图像
 * 
 * 主要功能：
 * - 16位灰度图像的显示和管理
 * - 窗宽窗位调整（手动/自动/ROI）
 * - ROI区域选择
 * - 鼠标交互（缩放、像素值显示）
 * - 图像文件的打开和保存
 */
class XGraphicsView : public QGraphicsView
{
	Q_OBJECT

public:
	XGraphicsView(QWidget* parent = nullptr);
	~XGraphicsView();

	/**
	 * @brief 更新显示的图像
	 * @param image 16位灰度图像 (Format_Grayscale16)
	 * @param adjustWL 是否重新计算窗宽窗位
	 */
	void updateImage(QImage image, bool adjustWL = false);
	
	/**
	 * @brief 更新ROI区域并调整窗宽窗位
	 * @param rect ROI区域矩形（场景坐标系）
	 */
	void updateRoiRect(QRectF rect);
	
	const QImage& getSrcU16Image() const;     ///< 获取当前原始图像
	void clearROIRect();                      ///< 清除ROI区域

public:
	void resetView();                         ///< 重置视野，适配图像大小
	void openImage();                         ///< 打开单个图像文件
	void openImageFolder();                   ///< 打开图像文件夹
	void saveImage();                         ///< 保存当前图像
	void setValidRectVisible(bool visible);   ///< 设置有效区域框显示
	void setCenterLinesVisible(bool visible); ///< 设置中心线显示
	void setROIEnable(bool enable);           ///< 启用/禁用ROI选择
	void setAutoWLEnable(bool enable);        ///< 启用/禁用自动窗位调整
	void setWindowLevel(int width, int level);///< 手动设置窗宽窗位
	bool isAutoWL() const;                    ///< 是否开启自动窗位
	void zoomIn();                            ///< 放大图像
	void zoomOut();                           ///< 缩小图像
	void showImage(int idx);                  ///< 显示指定索引的图像
	void clearCurrentImageList();             ///< 清空图像列表
	void addImageToList(QImage image);        ///< 添加图像到列表

private:
	void initContextMenu();                   ///< 初始化右键菜单
	void onWindowLevelChanged(int width, int level); ///< 窗宽窗位变化回调

signals:
	/**
	 * @brief 鼠标悬停在图像上时发出
	 * @param x 像素x坐标
	 * @param y 像素y坐标
	 * @param gray 16位灰度值 (0-65535)
	 */
	void signalPixelHovered(int x, int y, int gray);
	
	void signalMinMaxValueChanged(int min, int max);    ///< 图像最小/最大值变化
	void signalRoiWLChanged(int width, int level);      ///< ROI窗宽窗位变化
	void signalSrcU16ImageListSizeChanged(int n);       ///< 图像列表大小变化

protected:
	void mouseMoveEvent(QMouseEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	void wheelEvent(QWheelEvent* event) override;

private:
	// 图像数据
	QImage currentSrcU16Image;              ///< 当前显示的原始16位图像
	QList<QImage> srcU16ImageList;          ///< 图像列表（用于多帧显示）

	// 窗宽窗位管理器
	XWindowLevelManager* m_windowLevelManager{ nullptr };

	// Scene引用
	XGraphicsScene* xGraphicsScene{ nullptr };

	// ROI相关状态
	bool enableROI{ false };                ///< 是否启用ROI选择
	bool roiSeleting{ false };              ///< 是否正在选择ROI
	bool autoWL{ false };                   ///< 是否自动调整窗宽窗位
};

