#include "XGraphicsView.h"

#ifdef USE_ELA
#include "ElaTheme.h"
#endif // USE_ELA

#include <QtConcurrent/QtConcurrent>

#include <qevent.h>
#include <qdebug.h>
#include <qfiledialog.h>
#include <qthread.h>
#include <qmenu.h>
#include <qmessagebox.h>
#include <qaction.h>
#include <qcheckbox.h>
#include <qgraphicsitem.h>

#include "XGraphicsScene.h"
#include "XImageHelper.h"
#include "XWindowLevelManager.h"

#include "Components/XGlobal.h"
#include "Components/XFileHelper.h"
#include "Components/XSignalsHelper.h"

#include "../IRayDetector/TiffHelper.h"

XGraphicsView::XGraphicsView(QWidget* parent)
	: QGraphicsView(parent)
{
	this->setMouseTracking(true);
#ifdef USE_ELA
	//this->setBackgroundBrush(QColor(0x20, 0x20, 0x20));
	this->setBackgroundBrush(QColor(0x3e, 0x3e, 0x3e));
	connect(eTheme, &ElaTheme::themeModeChanged, this, [this](ElaThemeType::ThemeMode themeMode) {
		if (themeMode == ElaThemeType::ThemeMode::Dark)
		{
			//this->setBackgroundBrush(QColor(0x20, 0x20, 0x20));
			this->setBackgroundBrush(QColor(0x3e, 0x3e, 0x3e));
		}
		else
		{
			//this->setBackgroundBrush(QColor(0xF3, 0xF3, 0xF3));
			this->setBackgroundBrush(QColor(0xFF, 0xFF, 0xFF));
		}
		});
#else
	this->setBackgroundBrush(QColor::fromRgb(0x373c41));
#endif

	// 设置缩放锚点为鼠标位置
	this->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);

	// 创建Scene并设置
	xGraphicsScene = new XGraphicsScene();
	this->setScene(xGraphicsScene);

	// 创建窗宽窗位管理器
	m_windowLevelManager = new XWindowLevelManager(this);
	connect(m_windowLevelManager, &XWindowLevelManager::windowLevelChanged,
		this, &XGraphicsView::onWindowLevelChanged);

	initContextMenu();
}

XGraphicsView::~XGraphicsView()
{}



void XGraphicsView::updateImage(QImage image, bool adjustWL)
{
	if (image.format() != QImage::Format_Grayscale16)
	{
		qDebug() << "图像类型错误：" << image.format();
		return;
	}

	auto pixmapItem = xGraphicsScene->getPixmapItem();
	const bool bIsFirstImage = pixmapItem ? pixmapItem->pixmap().isNull() : true;
	currentSrcU16Image = image.copy();

	int max = -1;
	int min = -1;
	XImageHelper::calculateMaxMinValue(image, max, min);
	emit signalMinMaxValueChanged(min, max);
	qDebug() << "min max of srcU16Image: " << min << max;

	if (adjustWL || autoWL || bIsFirstImage)
	{
		m_windowLevelManager->calculateFromMinMax(min, max);
	}

	bool sizeChanged = false;
	if (pixmapItem && !pixmapItem->pixmap().isNull())
	{
		sizeChanged = pixmapItem->pixmap().size() != image.size();
	}

	// 通过Scene更新显示，传入原始图像和当前窗宽窗位
	xGraphicsScene->updatePixmapDisplay(currentSrcU16Image,
		m_windowLevelManager->getWidth(),
		m_windowLevelManager->getLevel());
	if (bIsFirstImage || sizeChanged)
	{
		resetView();
	}
}

void XGraphicsView::setValidRectVisible(bool visible)
{
	xGraphicsScene->setValidRectVisible(visible);
}

void XGraphicsView::setCenterLinesVisible(bool visible)
{
	xGraphicsScene->setCenterLinesVisible(visible);
}

void XGraphicsView::updateRoiRect(QRectF rect)
{
	xGraphicsScene->setROIRect(rect);
	xGraphicsScene->setROIVisible(enableROI);

	if (rect.width() < 10 || rect.height() < 10)
	{
		qDebug() << "选中的ROI区过小";
	}
	else
	{
		qDebug() << "ROI区域发生变化，当前窗宽：" << m_windowLevelManager->getWidth()
			<< "当前窗位：" << m_windowLevelManager->getLevel();
		m_windowLevelManager->calculateFromROI(currentSrcU16Image, rect.toRect());
		qDebug() << "ROI区域发生变化，目标窗宽：" << m_windowLevelManager->getWidth()
			<< "目标窗位：" << m_windowLevelManager->getLevel();
	}
}

void XGraphicsView::setROIEnable(bool enable)
{
	qDebug() << "设置允许ROI调整为：" << enable;
	enableROI = enable;

	this->setDragMode(enable ? QGraphicsView::DragMode::RubberBandDrag : QGraphicsView::DragMode::NoDrag);
	// 取消 ROI 区域的绘制
	if (!enable)
	{
		updateRoiRect(QRectF());
	}
}

void XGraphicsView::setAutoWLEnable(bool open)
{
	qDebug() << "设置自动窗宽窗位调整为：" << open;
	autoWL = open;
}

void XGraphicsView::setWindowLevel(int width, int level)
{
	m_windowLevelManager->setWindowLevel(width, level);
}

bool XGraphicsView::isAutoWL() const
{
	return autoWL;
}

void XGraphicsView::zoomIn()
{
	scale(1.25, 1.25);
}

void XGraphicsView::zoomOut()
{
	scale(0.8, 0.8);
}

void XGraphicsView::showImage(int idx)
{
	qDebug() << "切换当前显示的图片：" << idx << "，图像数组大小：" << srcU16ImageList.size();
	if (srcU16ImageList.size() > idx)
	{
		updateImage(srcU16ImageList.at(idx));
	}
}

void XGraphicsView::clearCurrentImageList()
{
	srcU16ImageList.clear();
}

void XGraphicsView::addImageToList(QImage image)
{
	srcU16ImageList.append(image);
}

void XGraphicsView::onWindowLevelChanged(int width, int level)
{
	// 当窗宽窗位改变时，更新Scene显示
	xGraphicsScene->updatePixmapDisplay(currentSrcU16Image, width, level);
	// 发出信号通知其他组件
	emit signalRoiWLChanged(width, level);
}

const QImage& XGraphicsView::getSrcU16Image() const
{
	return currentSrcU16Image;
}

void XGraphicsView::clearROIRect()
{
	xGraphicsScene->clearROIRect();
}

void XGraphicsView::resetView()
{
	auto pixmapItem = xGraphicsScene->getPixmapItem();
	if (pixmapItem)
	{
		// 将 pixmapItem 移动到视野中心
		resetTransform();
		qDebug() << pixmapItem->boundingRect();
		fitInView(pixmapItem->boundingRect(), Qt::KeepAspectRatio);
	}
}

void XGraphicsView::openImage()
{
	QString filePath = QFileDialog::getOpenFileName(nullptr,
		"打开图像文件",
		"",
		"RAW文件(*.raw);;TIFF文件(*.tif *.tiff)");
	
	if (filePath.isEmpty())
		return;
	
	QImage image;
	
	// 根据文件扩展名选择读取方式
	if (filePath.endsWith(".tif", Qt::CaseInsensitive) || 
		filePath.endsWith(".tiff", Qt::CaseInsensitive))
	{
		image = TiffHelper::ReadImage(filePath.toStdString());
		if (image.isNull())
		{
			emit xSignaHelper.signalShowErrorMessageBar("TIFF文件读取失败: " + filePath);
			return;
		}
	}
	else if (filePath.endsWith(".raw", Qt::CaseInsensitive))
	{
		image = XImageHelper::openImageU16Raw(filePath, DET_WIDTH, DET_HEIGHT);
		if (image.isNull())
		{
			emit xSignaHelper.signalShowErrorMessageBar("RAW文件读取失败: " + filePath);
			return;
		}
	}
	else
	{
		emit xSignaHelper.signalShowErrorMessageBar("不支持的文件格式: " + filePath);
		return;
	}
	
	srcU16ImageList.clear();
	srcU16ImageList.append(image);
	emit signalSrcU16ImageListSizeChanged(srcU16ImageList.size());
	updateImage(srcU16ImageList.first(), true);
}

void XGraphicsView::openImageFolder()
{
	QString folderPath = QFileDialog::getExistingDirectory(nullptr,
		"选择文件夹",
		QDir::homePath(),
		QFileDialog::ShowDirsOnly);

	if (folderPath.isEmpty() || folderPath.isNull())
		return;

	if (!QDir(folderPath).exists())
	{
		emit xSignaHelper.signalShowErrorMessageBar(QString("选择的目录不存在: ").arg(folderPath));
		return;
	}

	// 扫描文件夹，获取所有图像文件
	QDir dir(folderPath);
	QStringList filters;
	filters << "*.raw" << "*.tif" << "*.tiff";
	QFileInfoList fileList = dir.entryInfoList(filters, QDir::Files, QDir::Name);
	
	if (fileList.isEmpty())
	{
		emit xSignaHelper.signalShowErrorMessageBar("文件夹中没有找到图像文件（支持 .raw / .tif / .tiff）");
		return;
	}

	// 检查文件类型是否一致
	int rawCount = 0;
	int tiffCount = 0;
	
	for (const QFileInfo& fileInfo : fileList)
	{
		QString suffix = fileInfo.suffix().toLower();
		if (suffix == "raw")
			rawCount++;
		else if (suffix == "tif" || suffix == "tiff")
			tiffCount++;
	}
	
	if (rawCount > 0 && tiffCount > 0)
	{
		emit xSignaHelper.signalShowErrorMessageBar(
			QString("文件夹中包含混合格式（RAW: %1, TIFF: %2），请选择只包含单一格式的文件夹")
			.arg(rawCount).arg(tiffCount));
		return;
	}
	
	const bool isRawFormat = (rawCount > 0);
	const bool isTiffFormat = (tiffCount > 0);
	
	qDebug() << "开始读取图像文件夹，文件数量：" << fileList.size() 
		<< "，格式：" << (isRawFormat ? "RAW" : "TIFF");

	// 在后台线程中读取图像
	QFuture<QList<QImage>> future = QtConcurrent::run([fileList, isRawFormat]() -> QList<QImage> {
		QList<QImage> images;
		
		for (const QFileInfo& fileInfo : fileList)
		{
			QImage image;
			
			if (isRawFormat)
			{
				image = XImageHelper::openImageU16Raw(fileInfo.absoluteFilePath(), DET_WIDTH, DET_HEIGHT);
			}
			else  // TIFF format
			{
				image = TiffHelper::ReadImage(fileInfo.absoluteFilePath().toStdString());
			}
			
			if (!image.isNull())
			{
				images.append(image);
			}
			else
			{
				qDebug() << "读取文件失败：" << fileInfo.absoluteFilePath();
			}
		}
		
		return images;
	});

	// 连接完成信号
	QFutureWatcher<QList<QImage>>* watcher = new QFutureWatcher<QList<QImage>>(this);
	connect(watcher, &QFutureWatcher<QList<QImage>>::finished, this, [this, watcher, fileList]() {
		QList<QImage> result = watcher->future().result();
		
		if (result.isEmpty())
		{
			emit xSignaHelper.signalShowErrorMessageBar("未能成功读取任何图像文件");
		}
		else
		{
			srcU16ImageList.clear();
			srcU16ImageList = result;
			emit signalSrcU16ImageListSizeChanged(srcU16ImageList.size());
			updateImage(srcU16ImageList.first(), true);
			
			qDebug() << "成功读取" << result.size() << "/" << fileList.size() << "个图像文件";
		}
		
		watcher->deleteLater();
	});

	watcher->setFuture(future);
}

void XGraphicsView::saveImage()
{
	if (currentSrcU16Image.isNull())
	{
		//QMessageBox::critical(nullptr, 
		//	"错误",
		//	"当前没有正在显示的图像",
		//	"确定");
		emit xSignaHelper.signalShowErrorMessageBar("当前没有正在显示的图像");
		return;
	}
	// 获取保存文件的路径和格式
	QFileInfo fileInfo = XFileHelper::getImageSaveFileInfo();
	if (fileInfo.absoluteFilePath().endsWith(".raw"))
	{
		XImageHelper::Instance().saveImageU16Raw(currentSrcU16Image, fileInfo.absoluteFilePath());
	}
	else if (fileInfo.absoluteFilePath().endsWith(".tif"))
	{
		TiffHelper::SaveImage(currentSrcU16Image, fileInfo.absoluteFilePath().toStdString());
	}
	else if (fileInfo.absoluteFilePath().endsWith(".png"))
	{
		// 使用Scene提供的pixmap进行保存
		auto pixmapItem = xGraphicsScene->getPixmapItem();
		if (pixmapItem && !pixmapItem->pixmap().isNull())
		{
			XImageHelper::Instance().saveImagePNG(pixmapItem->pixmap().toImage(), fileInfo.absoluteFilePath());
		}
	}
	else if (fileInfo.absoluteFilePath().endsWith(".jpg") ||
		fileInfo.absoluteFilePath().endsWith(".jpeg"))
	{
		// 使用Scene提供的pixmap进行保存
		auto pixmapItem = xGraphicsScene->getPixmapItem();
		if (pixmapItem && !pixmapItem->pixmap().isNull())
		{
			XImageHelper::Instance().saveImageJPG(pixmapItem->pixmap().toImage(), fileInfo.absoluteFilePath());
		}
	}
}

void XGraphicsView::initContextMenu()
{
	setContextMenuPolicy(Qt::CustomContextMenu);

	QMenu* menu = new QMenu(this);

	QAction* zoomOut = new QAction("缩小");
	zoomOut->setCheckable(false);
	menu->addAction(zoomOut);

	QAction* zoomIn = new QAction("放大");
	zoomIn->setCheckable(false);
	menu->addAction(zoomIn);

	QAction* resetViewAction = new QAction("重置视野");
	resetViewAction->setCheckable(false);
	menu->addAction(resetViewAction);

	menu->addSeparator();

	QAction* openFileAction = new QAction("打开文件");
	openFileAction->setCheckable(false);
	menu->addAction(openFileAction);
	openFileAction->setVisible(false);

	QAction* openFileFolderAction = new QAction("打开文件夹");
	openFileFolderAction->setCheckable(false);
	menu->addAction(openFileFolderAction);
	openFileFolderAction->setVisible(false);

	QAction* saveAsAction = new QAction("另存为");
	saveAsAction->setCheckable(false);
	menu->addAction(saveAsAction);

	menu->addSeparator();

	QAction* showValidRectAction = new QAction("显示探测器有效视野");
	showValidRectAction->setCheckable(true);
	menu->addAction(showValidRectAction);

	QAction* showCenterLieAction = new QAction("绘制视野中心线");
	showCenterLieAction->setCheckable(true);
	menu->addAction(showCenterLieAction);

	menu->addSeparator();

	QAction* roiWLAction = new QAction("ROI调整窗宽窗位");
	roiWLAction->setCheckable(true);
	menu->addAction(roiWLAction);

	QAction* autoWLAction = new QAction("自动调整窗宽窗位");
	autoWLAction->setCheckable(true);
	menu->addAction(autoWLAction);

	connect(zoomIn, &QAction::triggered, this, &XGraphicsView::zoomIn);
	connect(zoomOut, &QAction::triggered, this, &XGraphicsView::zoomOut);
	connect(resetViewAction, &QAction::triggered, this, &XGraphicsView::resetView);
	connect(openFileAction, &QAction::triggered, this, &XGraphicsView::openImage);
	connect(openFileFolderAction, &QAction::triggered, this, &XGraphicsView::openImageFolder);
	connect(saveAsAction, &QAction::triggered, this, &XGraphicsView::saveImage);

	connect(showValidRectAction, &QAction::triggered, this, [this](bool checked) {
		xGraphicsScene->setValidRectVisible(checked);
		});

	connect(showCenterLieAction, &QAction::triggered, this, [this](bool checked) {
		xGraphicsScene->setCenterLinesVisible(checked);
		});

	connect(roiWLAction, &QAction::triggered, this, [this, autoWLAction](bool checked) {
		setROIEnable(checked);
		if (checked && autoWLAction->isChecked())
		{
			autoWLAction->setChecked(false);
			setAutoWLEnable(false);
		}
		});

	connect(autoWLAction, &QAction::triggered, this, [this, roiWLAction](bool checked) {
		setAutoWLEnable(checked);
		if (checked && roiWLAction->isChecked())
		{
			roiWLAction->setChecked(false);
			setROIEnable(false);
		}
		});

	connect(this, &QGraphicsView::customContextMenuRequested, this, [this, menu](const QPoint& pos) {
		// 显示菜单
		menu->exec(mapToGlobal(pos));
		});
}

void XGraphicsView::mouseMoveEvent(QMouseEvent* event)
{
	QPoint viewPos = event->pos();
	QPointF scenePos = mapToScene(viewPos);
	// 转换为图像项坐标
	auto pixmapItem = xGraphicsScene->getPixmapItem();
	if (!pixmapItem) {
		QGraphicsView::mouseMoveEvent(event);
		return;
	}
	QPointF itemPos = pixmapItem->mapFromScene(scenePos);
	int x = static_cast<int>(itemPos.x());
	int y = static_cast<int>(itemPos.y());

	const QImage& image = currentSrcU16Image;
	// 检查坐标是否在图像范围内
	if (x >= 0 && x < image.width() && y >= 0 && y < image.height())
	{
		// 获取像素值
		QRgb pixel = image.pixel(x, y);

		// 获取该行数据的常量指针，并重新解释为 ushort 指针
		const uchar* scanLine = image.constScanLine(y); // 获取第y行数据的起始地址
		const ushort* pixelData = reinterpret_cast<const ushort*>(scanLine); // 解释为16位数据

		// 读取该像素的16位灰度值
		ushort grayValue16 = pixelData[x]; // 范围是 0 到 65535

		// 发送信号通知其他组件
		emit signalPixelHovered(x, y, grayValue16);
	}

	QGraphicsView::mouseMoveEvent(event);  // 调用基类实现
}

void XGraphicsView::mousePressEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton) {
		qDebug() << "鼠标左键按下";
	}

	// 重要：调用基类事件，确保场景原有的交互（如项目选择）仍能正常工作
	QGraphicsView::mousePressEvent(event);
}

void XGraphicsView::mouseReleaseEvent(QMouseEvent* event)
{
	if (this->dragMode() == QGraphicsView::DragMode::RubberBandDrag && event->button() == Qt::LeftButton) {
		qDebug() << "鼠标左键松开";
		roiSeleting = false;

		auto _sceneRect = this->sceneRect().adjusted(0.5, 0.5, 0, 0);
		auto _rubbeBandRect = mapToScene(rubberBandRect()).boundingRect();
		auto validROIRect = _sceneRect.intersected(_rubbeBandRect);
		qDebug() << _sceneRect << _rubbeBandRect << validROIRect;

		// 绘制完成，发出信号（例如，用于后续ROI处理）
		if (!validROIRect.isEmpty() && validROIRect.isValid()) {
			qDebug() << "选中的ROI区域为：" << validROIRect;
			updateRoiRect(validROIRect);
		}
		else
		{
			// 清除临时矩形
			validROIRect = QRectF();
			updateRoiRect(validROIRect);
		}
	}
	QGraphicsView::mouseReleaseEvent(event);
}

void XGraphicsView::wheelEvent(QWheelEvent* event)
{
	// 检查是否按下Ctrl键
	if (event->modifiers() & Qt::ControlModifier) {
		// 处理缩放
		if (event->angleDelta().y() > 0) {
			// 滚轮向上，放大
			zoomIn();
		}
		else {
			// 滚轮向下，缩小
			zoomOut();
		}
		event->accept();
	}
	else {
		// 未按下Ctrl，执行默认滚动行为
		QGraphicsView::wheelEvent(event);
	}
}
