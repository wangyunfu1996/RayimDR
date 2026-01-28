#include "XGraphicsView.h"

#ifdef USE_ELA
#include "ElaTheme.h"
#endif  // USE_ELA

#include <QtConcurrent/QtConcurrent>

#include <qevent.h>
#include <qdebug.h>
#include <qfiledialog.h>
#include <qinputdialog.h>
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

#include "IRayDetector/TiffHelper.h"

XGraphicsView::XGraphicsView(QWidget* parent) : QGraphicsView(parent)
{
    this->setMouseTracking(true);
#ifdef USE_ELA
    // this->setBackgroundBrush(QColor(0x20, 0x20, 0x20));
    this->setBackgroundBrush(QColor(0x3e, 0x3e, 0x3e));
    connect(eTheme, &ElaTheme::themeModeChanged, this,
            [this](ElaThemeType::ThemeMode themeMode)
            {
                if (themeMode == ElaThemeType::ThemeMode::Dark)
                {
                    // this->setBackgroundBrush(QColor(0x20, 0x20, 0x20));
                    this->setBackgroundBrush(QColor(0x3e, 0x3e, 0x3e));
                }
                else
                {
                    // this->setBackgroundBrush(QColor(0xF3, 0xF3, 0xF3));
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
    connect(m_windowLevelManager, &XWindowLevelManager::windowLevelChanged, this, &XGraphicsView::onWindowLevelChanged);

    initContextMenu();
}

XGraphicsView::~XGraphicsView() {}

void XGraphicsView::updateImage(QImage image, bool adjustWL)
{
    if (image.format() != QImage::Format_Grayscale16)
    {
        qDebug() << "图像类型错误：" << image.format();
        return;
    }

    auto pixmapItem = xGraphicsScene->getPixmapItem();
    const bool bIsFirstImage = pixmapItem ? pixmapItem->pixmap().isNull() : true;

    // 检测尺寸是否变化
    const bool sizeChanged =
        pixmapItem && !pixmapItem->pixmap().isNull() && pixmapItem->pixmap().size() != image.size();

    if (sizeChanged)
    {
        qDebug() << "图像尺寸变化 - 旧尺寸:" << pixmapItem->pixmap().size() << "新尺寸:" << image.size();
    }

    // 存储图像（使用移动语义避免深拷贝）
    currentSrcU16Image = std::move(image);

    // 计算并发送最小最大值
    int max = -1;
    int min = -1;
    XImageHelper::calculateMaxMinValue(currentSrcU16Image, max, min);
    emit signalMinMaxValueChanged(min, max);

    qDebug() << "图像统计 - Min:" << min << "Max:" << max << "Size:" << currentSrcU16Image.width() << "x"
             << currentSrcU16Image.height();

    // 根据需要调整窗宽窗位
    if (adjustWL || autoWL || bIsFirstImage)
    {
        m_windowLevelManager->calculateFromMinMax(min, max);
    }

    // 通过Scene更新显示
    xGraphicsScene->updatePixmapDisplay(currentSrcU16Image, m_windowLevelManager->getWidth(),
                                        m_windowLevelManager->getLevel());

    // 首次加载或尺寸变化时重置视野
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

    // ROI区域有效性检查
    if (!rect.isEmpty() && rect.isValid())
    {
        const qreal minSize = 10.0;
        if (rect.width() < minSize || rect.height() < minSize)
        {
            qDebug() << "ROI区域过小：" << rect;
            return;
        }

        // 根据ROI区域调整窗宽窗位
        const int oldWidth = m_windowLevelManager->getWidth();
        const int oldLevel = m_windowLevelManager->getLevel();

        m_windowLevelManager->calculateFromROI(currentSrcU16Image, rect.toRect());

        qDebug() << "ROI窗宽窗位调整 -"
                 << "从 W:" << oldWidth << "L:" << oldLevel << "到 W:" << m_windowLevelManager->getWidth()
                 << "L:" << m_windowLevelManager->getLevel();
    }
}

void XGraphicsView::setROIEnable(bool enable)
{
    if (enableROI == enable)
        return;

    enableROI = enable;
    qDebug() << "ROI调整窗宽窗位:" << (enable ? "开启" : "关闭");

    this->setDragMode(enable ? QGraphicsView::DragMode::RubberBandDrag : QGraphicsView::DragMode::NoDrag);

    // 关闭时清除 ROI 区域
    if (!enable)
    {
        updateRoiRect(QRectF());
    }
}

void XGraphicsView::setAutoWLEnable(bool open)
{
    if (autoWL == open)
        return;

    autoWL = open;
    qDebug() << "自动窗宽窗位调整:" << (open ? "开启" : "关闭");
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
    if (idx < 0 || idx >= srcU16ImageList.size())
    {
        qDebug() << "图像索引越界：" << idx << "/" << srcU16ImageList.size();
        return;
    }

    qDebug() << "显示图像 [" << idx << "/" << srcU16ImageList.size() << "]";
    updateImage(srcU16ImageList.at(idx));
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
    if (pixmapItem && !pixmapItem->pixmap().isNull())
    {
        // 重置变换矩阵并将图像适配到视野中心
        resetTransform();
        const QRectF bounds = pixmapItem->boundingRect();
        const QRectF sceneRect = xGraphicsScene->sceneRect();

        qDebug() << "重置视野 - Pixmap边界:" << bounds << "场景矩形:" << sceneRect
                 << "Pixmap大小:" << pixmapItem->pixmap().size();

        // 使用 pixmapItem 的边界进行 fitInView，确保图像居中显示
        fitInView(bounds, Qt::KeepAspectRatio);
    }
}

void XGraphicsView::openImage()
{
    const QString filePath = QFileDialog::getOpenFileName(
        nullptr, "打开图像文件", "", "RAW文件(*.raw);;TIFF文件(*.tif *.tiff);;所有支持的文件(*.raw *.tif *.tiff)");

    if (filePath.isEmpty())
        return;

    QImage image;
    const QString suffix = QFileInfo(filePath).suffix().toLower();

    // 根据文件扩展名选择读取方式
    if (suffix == "tif" || suffix == "tiff")
    {
        image = TiffHelper::ReadImage(filePath.toStdString());
    }
    else if (suffix == "raw")
    {
        // RAW 文件需要用户输入图像尺寸
        bool okWidth = false;
        int width = QInputDialog::getInt(nullptr, "输入图像宽度", "请输入图像宽度（像素）:", DET_WIDTH_1X1, 1, 65536, 1,
                                         &okWidth);
        if (!okWidth)
            return;

        bool okHeight = false;
        int height = QInputDialog::getInt(nullptr, "输入图像高度", "请输入图像高度（像素）:", DET_HEIGHT_1X1, 1, 65536,
                                          1, &okHeight);
        if (!okHeight)
            return;

        image = XImageHelper::openImageU16Raw(filePath, width, height);
    }
    else
    {
        emit xSignaHelper.signalShowErrorMessageBar("不支持的文件格式: " + suffix);
        return;
    }

    if (image.isNull())
    {
        emit xSignaHelper.signalShowErrorMessageBar("文件读取失败: " + filePath);
        return;
    }

    // 更新图像列表
    srcU16ImageList.clear();
    srcU16ImageList.append(std::move(image));
    emit signalSrcU16ImageListSizeChanged(srcU16ImageList.size());

    // 显示第一张图像并调整窗位
    updateImage(srcU16ImageList.first(), true);
}

void XGraphicsView::openImageFolder()
{
    QString folderPath =
        QFileDialog::getExistingDirectory(nullptr, "选择文件夹", QDir::homePath(), QFileDialog::ShowDirsOnly);

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
                .arg(rawCount)
                .arg(tiffCount));
        return;
    }

    const bool isRawFormat = (rawCount > 0);
    const bool isTiffFormat = (tiffCount > 0);

    // 如果是 RAW 格式，需要用户输入图像尺寸
    int rawWidth = DET_WIDTH_1X1;
    int rawHeight = DET_HEIGHT_1X1;
    if (isRawFormat)
    {
        bool okWidth = false;
        rawWidth = QInputDialog::getInt(nullptr, "输入图像宽度", "请输入 RAW 图像宽度（像素）:", DET_WIDTH_1X1, 1,
                                        65536, 1, &okWidth);
        if (!okWidth)
            return;

        bool okHeight = false;
        rawHeight = QInputDialog::getInt(nullptr, "输入图像高度", "请输入 RAW 图像高度（像素）:", DET_HEIGHT_1X1, 1,
                                         65536, 1, &okHeight);
        if (!okHeight)
            return;
    }

    qDebug() << "开始读取图像文件夹，文件数量：" << fileList.size() << "，格式：" << (isRawFormat ? "RAW" : "TIFF");

    // 在后台线程中读取图像
    QFuture<QList<QImage>> future = QtConcurrent::run(
        [fileList, isRawFormat, rawWidth, rawHeight]() -> QList<QImage>
        {
            QList<QImage> images;

            for (const QFileInfo& fileInfo : fileList)
            {
                QImage image;

                if (isRawFormat)
                {
                    image = XImageHelper::openImageU16Raw(fileInfo.absoluteFilePath(), rawWidth, rawHeight);
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
    connect(watcher, &QFutureWatcher<QList<QImage>>::finished, this,
            [this, watcher, fileList]()
            {
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
        emit xSignaHelper.signalShowErrorMessageBar("当前没有正在显示的图像");
        return;
    }

    // 获取保存文件的路径和格式
    const QFileInfo fileInfo = XFileHelper::getImageSaveFileInfo();
    const QString filePath = fileInfo.absoluteFilePath();

    if (filePath.isEmpty())
        return;

    const QString suffix = fileInfo.suffix().toLower();

    // 根据文件类型保存
    if (suffix == "raw")
    {
        XImageHelper::Instance().saveImageU16Raw(currentSrcU16Image, filePath);
    }
    else if (suffix == "tif" || suffix == "tiff")
    {
        TiffHelper::SaveImage(currentSrcU16Image, filePath.toStdString());
    }
    else if (suffix == "png" || suffix == "jpg" || suffix == "jpeg")
    {
        // 使用Scene提供的显示pixmap进行保存
        auto pixmapItem = xGraphicsScene->getPixmapItem();
        if (!pixmapItem || pixmapItem->pixmap().isNull())
        {
            emit xSignaHelper.signalShowErrorMessageBar("无法获取显示图像");
            return;
        }

        const QImage displayImage = pixmapItem->pixmap().toImage();
        if (suffix == "png")
        {
            XImageHelper::Instance().saveImagePNG(displayImage, filePath);
        }
        else
        {
            XImageHelper::Instance().saveImageJPG(displayImage, filePath);
        }
    }
    else
    {
        emit xSignaHelper.signalShowErrorMessageBar("不支持的保存格式: " + suffix);
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

    connect(showValidRectAction, &QAction::triggered, this,
            [this](bool checked) { xGraphicsScene->setValidRectVisible(checked); });

    connect(showCenterLieAction, &QAction::triggered, this,
            [this](bool checked) { xGraphicsScene->setCenterLinesVisible(checked); });

    connect(roiWLAction, &QAction::triggered, this,
            [this, autoWLAction](bool checked)
            {
                setROIEnable(checked);
                if (checked && autoWLAction->isChecked())
                {
                    autoWLAction->setChecked(false);
                    setAutoWLEnable(false);
                }
            });

    connect(autoWLAction, &QAction::triggered, this,
            [this, roiWLAction](bool checked)
            {
                setAutoWLEnable(checked);
                if (checked && roiWLAction->isChecked())
                {
                    roiWLAction->setChecked(false);
                    setROIEnable(false);
                }
            });

    connect(this, &QGraphicsView::customContextMenuRequested, this,
            [this, menu](const QPoint& pos)
            {
                // 显示菜单
                menu->exec(mapToGlobal(pos));
            });
}

void XGraphicsView::mouseMoveEvent(QMouseEvent* event)
{
    // 获取图像项
    auto pixmapItem = xGraphicsScene->getPixmapItem();
    if (!pixmapItem || currentSrcU16Image.isNull())
    {
        QGraphicsView::mouseMoveEvent(event);
        return;
    }

    // 坐标转换：视图 -> 场景 -> 图像项
    const QPointF scenePos = mapToScene(event->pos());
    const QPointF itemPos = pixmapItem->mapFromScene(scenePos);
    const int x = static_cast<int>(itemPos.x());
    const int y = static_cast<int>(itemPos.y());

    // 检查坐标是否在图像范围内
    if (x >= 0 && x < currentSrcU16Image.width() && y >= 0 && y < currentSrcU16Image.height())
    {
        // 直接读取16位灰度值
        const ushort* scanLine = reinterpret_cast<const ushort*>(currentSrcU16Image.constScanLine(y));
        const ushort grayValue16 = scanLine[x];

        // 发送像素信息信号
        emit signalPixelHovered(x, y, grayValue16);
    }

    QGraphicsView::mouseMoveEvent(event);
}

void XGraphicsView::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        qDebug() << "鼠标左键按下";
    }

    // 重要：调用基类事件，确保场景原有的交互（如项目选择）仍能正常工作
    QGraphicsView::mousePressEvent(event);
}

void XGraphicsView::mouseReleaseEvent(QMouseEvent* event)
{
    if (this->dragMode() == QGraphicsView::DragMode::RubberBandDrag && event->button() == Qt::LeftButton)
    {
        roiSeleting = false;

        // 计算有效的ROI区域（橡皮筋矩形与场景的交集）
        const QRectF sceneRect = this->sceneRect();
        const QRectF rubberBandSceneRect = mapToScene(rubberBandRect()).boundingRect();
        const QRectF validROIRect = sceneRect.intersected(rubberBandSceneRect);

        qDebug() << "ROI选择 - 场景:" << sceneRect << "橡皮筋:" << rubberBandSceneRect << "有效区域:" << validROIRect;

        // 更新ROI矩形（无论是否为空）
        updateRoiRect(validROIRect);
    }

    QGraphicsView::mouseReleaseEvent(event);
}

void XGraphicsView::wheelEvent(QWheelEvent* event)
{
    // Ctrl+滚轮：缩放图像
    if (event->modifiers() & Qt::ControlModifier)
    {
        if (event->angleDelta().y() > 0)
        {
            zoomIn();  // 滚轮向上，放大
        }
        else
        {
            zoomOut();  // 滚轮向下，缩小
        }
        event->accept();
    }
    else
    {
        // 未按Ctrl，执行默认滚动行为
        QGraphicsView::wheelEvent(event);
    }
}
