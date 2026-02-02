#include "MainWindow.h"

#include <QtConcurrent/QtConcurrent>

#include <qtimer.h>
#include <qdebug.h>
#include <qlayout.h>
#include <qthread.h>
#include <qfuture.h>
#include <qfuturewatcher.h>
#include <qfutureinterface.h>

#include "ElaContentDialog.h"
#include "ElaTheme.h"
#include "ElaMenuBar.h"
#include "ElaMenu.h"
#include "ElaToolBar.h"
#include "ElaToolButton.h"
#include "ElaStatusBar.h"
#include "ElaText.h"
#include "ElaMessageBar.h"
#include "ElaTabWidget.h"
#include "ElaSpinBox.h"
#include "ElaDialog.h"

#include "Components/XSignalsHelper.h"
#include "Components/AcqTaskManager.h"
#include "Components/XGlobal.h"

#include "ImageRender/XGraphicsView.h"
#include "ImageRender/XImageAdjustTool.h"
#include "ImageRender/XImageHelper.h"

#include "UI/XElaDialog.h"
#include "UI/CommonConfigUI.h"
#include "UI/XRayCfgDialog.h"
#include "UI/CreateCorrectTemplateDlg.h"
#include "UI/AppCfgDialog.h"
#include "UI/MultiAcqCfgDialog.h"

#include "IRayDetector/NDT1717MA.h"
#include "IRayDetector/IRayDetectorWidget.h"
#include "IRayDetector/QtLogger.h"

#include "VJXRAY/IXS120BP120P366.h"

MainWindow::MainWindow(QWidget* parent) : ElaWindow(parent)
{
    setFocusPolicy(Qt::StrongFocus);

    setWindowIcon(QPixmap(":/Resource/Image/logo_32x32.ico"));

    setIsCentralStackedWidgetTransparent(true);
    eTheme->setThemeMode(ElaThemeType::Dark);
    setIsNavigationBarEnable(false);
    setWindowTitle("锐影DR检测软件");
    setUserInfoCardVisible(false);

    setWindowButtonFlag(ElaAppBarType::RouteBackButtonHint, false);
    setWindowButtonFlag(ElaAppBarType::RouteForwardButtonHint, false);
    setWindowButtonFlag(ElaAppBarType::NavigationButtonHint, false);
    setWindowButtonFlag(ElaAppBarType::ThemeChangeButtonHint, false);

    // 拦截默认关闭事件
    _closeDialog = new ElaContentDialog(this);
    _closeDialog->setLeftButtonText("取消");
    _closeDialog->setMiddleButtonText("最小化");
    _closeDialog->setRightButtonText("确认");

    connect(_closeDialog, &ElaContentDialog::rightButtonClicked, this,
            [this]()
            {
                DET.StopQueryStatus();
                DET.DeInitialize();
                this->close();
            });

    this->setIsDefaultClosed(false);
    connect(this, &MainWindow::closeButtonClicked, this,
            [=]()
            {
                // 检查当前是否存在正在执行的采集任务
                if (AcqTaskManager::Instance().isAcquiring())
                {
                    XElaDialog dialog("请先取消当前正在进行的采集任务!", XElaDialogType::ERR);
                    dialog.showCentered();
                    return;
                }

                _closeDialog->exec();
            });

    QWidget* mainWidget = new QWidget(this);
    _XGraphicsView = new XGraphicsView(mainWidget);
    _XImageAdjustTool = new XImageAdjustTool(mainWidget);
    _CommonConfigUI = new CommonConfigUI(mainWidget);
    initMenuBar();

    initToolBar();

    // 状态栏
    ElaStatusBar* statusBar = new ElaStatusBar(this);
    _statusText = new ElaText(QDateTime::currentDateTime().toString(), statusBar);
    _statusText->setTextPixelSize(15);
    statusBar->addWidget(_statusText, 1);
    this->setStatusBar(statusBar);

    connect(&xSignaHelper, &XSignalsHelper::signalUpdateStatusInfo, this, &MainWindow::updateStatusText);
    connect(&xSignaHelper, &XSignalsHelper::signalShowErrorMessageBar, this,
            [this](const QString& msg) { ElaMessageBar::error(ElaMessageBarType::TopRight, "错误", msg, 4000); });
    connect(&xSignaHelper, &XSignalsHelper::signalShowSuccessMessageBar, this,
            [this](const QString& msg) { ElaMessageBar::success(ElaMessageBarType::TopRight, "成功", msg, 4000); });

    QVBoxLayout* vLayout = new QVBoxLayout;
    vLayout->addWidget(_XGraphicsView);
    vLayout->addWidget(_XImageAdjustTool);
    vLayout->setStretch(0, 1);
    vLayout->setStretch(1, 0);

    QHBoxLayout* hLayout = new QHBoxLayout;
    hLayout->addLayout(vLayout);
    hLayout->addWidget(_CommonConfigUI);
    hLayout->setStretch(0, 7);
    hLayout->setStretch(1, 3);

    mainWidget->setLayout(hLayout);

    addPageNode("HOME", mainWidget, ElaIconType::House);

    connect(_XImageAdjustTool, &XImageAdjustTool::signalAdjustWL, _XGraphicsView, &XGraphicsView::setWindowLevel);
    connect(_XImageAdjustTool, &XImageAdjustTool::signalSetROIEnable, _XGraphicsView, &XGraphicsView::setROIEnable);
    connect(_XImageAdjustTool, &XImageAdjustTool::signalAutoWL, _XGraphicsView, &XGraphicsView::setAutoWLEnable);
    connect(_XImageAdjustTool, &XImageAdjustTool::signalImageIdxChanged, _XGraphicsView, &XGraphicsView::showImage);
    connect(_XGraphicsView, &XGraphicsView::signalSrcU16ImageListSizeChanged, _XImageAdjustTool,
            &XImageAdjustTool::updateIdxRange);
    connect(_XGraphicsView, &XGraphicsView::signalMinMaxValueChanged, _XImageAdjustTool,
            &XImageAdjustTool::updateWLRange);
    connect(_XGraphicsView, &XGraphicsView::signalPixelHovered, _XImageAdjustTool,
            &XImageAdjustTool::updatePixelValueInfo);
    connect(_XGraphicsView, &XGraphicsView::signalRoiWLChanged, _XImageAdjustTool, &XImageAdjustTool::setWLValue);

    connect(&AcqTaskManager::Instance(), &AcqTaskManager::signalAcqTaskStopped, this, &MainWindow::onAcqStopped);
    connect(&AcqTaskManager::Instance(), &AcqTaskManager::signalAcqTaskReceivedIdxChanged, this,
            &MainWindow::onAcqImageReceived);
    connect(&AcqTaskManager::Instance(), &AcqTaskManager::signalAcqStatusMsg, this,
            [this](const QString& msg, int ec)
            {
                updateStatusText(msg);
                if (ec == 1)
                    ElaMessageBar::error(ElaMessageBarType::TopRight, "错误", msg, 4000);
            });

    connect(&XImageHelper::Instance(), &XImageHelper::signalOpenImageFolderProgressChanged, this,
            [this](int progress) { updateStatusText(QString("数据读取中，处理进度: %1%").arg(progress)); });

    QTimer::singleShot(500, this, &MainWindow::connectToDevices);
}

MainWindow::~MainWindow()
{
    qDebug() << "主窗口析构";
}

void MainWindow::updateStatusText(const QString& msg)
{
    qDebug() << msg;
    _statusText->setText(msg);
}

void MainWindow::onAcqStarted(const AcqCondition& acqCond)
{
    toolButtonImport->setEnabled(false);
    toolButtonOpenFolder->setEnabled(false);
    toolButtonSaveFile->setEnabled(false);

    toolButtonDR->setEnabled(false);
    toolButtonDRMulti->setEnabled(false);
    toolButtonRealTimeDR->setEnabled(false);

    _XGraphicsView->clearROIRect();
    _XGraphicsView->clearCurrentImageList();

    // （单张和连续），不启用图像索引的sliderIdx
    if (acqCond.frame == 1 || acqCond.frame == INT_MAX)
    {
        _XImageAdjustTool->updateIdxRange(0);
    }
    // 如果是多张采集，开启图像索引的sliderIdx
    else
    {
        _XImageAdjustTool->updateIdxRange(acqCond.frame);
    }

    updateStatusText("开始采集");
}

void MainWindow::onAcqStopped()
{
    toolButtonImport->setEnabled(true);
    toolButtonOpenFolder->setEnabled(true);
    toolButtonSaveFile->setEnabled(true);

    toolButtonDR->setEnabled(true);
    toolButtonDRMulti->setEnabled(true);
    toolButtonRealTimeDR->setEnabled(true);
}

void MainWindow::onAcqImageReceived(AcqCondition condition, int receivedIdx)
{
    qDebug() << "展示第 " << receivedIdx << "帧数据 采集条件：" << condition;

    if (condition.acqType == AcqType::DR)
    {
        _XGraphicsView->updateImage(AcqTaskManager::Instance().receivedImage(receivedIdx));
        if (condition.frame == 1)
        {
            updateStatusText("单张采集结束");
        }
        else if (condition.frame == INT_MAX)
        {
            updateStatusText(QString("连续DR采集，当前接收帧数：%1").arg(receivedIdx + 1));
        }
        else
        {
            _XGraphicsView->addImageToList(AcqTaskManager::Instance().receivedImage(receivedIdx));
            updateStatusText(
                QString("多张DR采集，当前接收帧数：%1，共 %2 帧").arg(receivedIdx + 1).arg(condition.frame));
        }
    }
}

void MainWindow::onDROnceTimeBtnClicked()
{
    // TODO 检查开光状态
    if (!_CommonConfigUI->checkInputValid())
    {
        qDebug() << "进行扫描前的参数检查失败！";
        return;
    }

    auto acqCond = _CommonConfigUI->getAcqCondition();
    acqCond.acqType = AcqType::DR;
    acqCond.frame = 1;

    AcqTaskManager::Instance().updateAcqCond(acqCond);
    AcqTaskManager::Instance().startAcq();
    onAcqStarted(acqCond);
}

void MainWindow::onDRMutliBtnClicked()
{
    // TODO 检查开光状态
    if (!_CommonConfigUI->checkInputValid())
    {
        qDebug() << "进行扫描前的参数检查失败！";
        return;
    }

    MultiAcqCfgDialog dialog;
    if (dialog.exec() == QDialog::Accepted)
    {
        qDebug() << "Accepted";
        AcqCondition acqCond = _CommonConfigUI->getAcqCondition();
        acqCond.acqType = AcqType::DR;

        if (!dialog.getCfg(acqCond.frame, acqCond.saveToFiles, acqCond.savePath, acqCond.saveType))
        {
            // emit xSignaHelper.signalShowErrorMessageBar("获取多帧采集参数错误！");
            qDebug() << "获取多帧采集参数错误!";
            qDebug() << acqCond;
            return;
        }

        AcqTaskManager::Instance().updateAcqCond(acqCond);
        AcqTaskManager::Instance().startAcq();
        onAcqStarted(acqCond);
    }
    else
    {
        qDebug() << "Rejected";
    }
}

void MainWindow::onDRRealTimeBtnClicked()
{
    // TODO 检查开光状态
    if (!_CommonConfigUI->checkInputValid())
    {
        qDebug() << "进行扫描前的参数检查失败！";
        return;
    }

    auto acqCond = _CommonConfigUI->getAcqCondition();
    acqCond.acqType = AcqType::DR;
    acqCond.frame = INT_MAX;

    AcqTaskManager::Instance().updateAcqCond(acqCond);
    AcqTaskManager::Instance().startAcq();

    onAcqStarted(acqCond);
}

void MainWindow::onDRStopBtnClicked()
{
    AcqTaskManager::Instance().stopAcq();
}

void MainWindow::initMenuBar()
{
    // 菜单栏
    ElaMenuBar* menuBar = new ElaMenuBar(this);
    this->setMenuBar(menuBar);
    ElaMenu* fileMenu = menuBar->addMenu("文件");
    QAction* action = fileMenu->addAction("打开图像文件");
    connect(action, &QAction::triggered, _XGraphicsView, &XGraphicsView::openImage);

    action = fileMenu->addAction("打开图像文件夹");
    connect(action, &QAction::triggered, _XGraphicsView, &XGraphicsView::openImageFolder);

    action = fileMenu->addAction("保存");
    connect(action, &QAction::triggered, _XGraphicsView, &XGraphicsView::saveImage);

    fileMenu->addSeparator();

    action = fileMenu->addAction("退出");
    connect(action, &QAction::triggered, this, [this]() { _closeDialog->exec(); });

    ElaMenu* configMenu = menuBar->addMenu("设置");
    action = configMenu->addAction("射线源设置");
    connect(action, &QAction::triggered, this,
            [this]()
            {
                XRayCfgDialog dialog;
                dialog.exec();
            });

    action = configMenu->addAction("探测器校正");
    connect(action, &QAction::triggered, this,
            [this]()
            {
                if (!DET.Initialized())
                {
                    emit xSignaHelper.signalShowErrorMessageBar("探测器未连接!");
                    return;
                }

                CreateCorrectTemplateDlg dialog;
                dialog.exec();
            });

    action = configMenu->addAction("清理日志");
    connect(action, &QAction::triggered, this,
            [this]()
            {
                if (QtLogger::cleanupLogs())
                {
                    emit xSignaHelper.signalShowSuccessMessageBar("日志已清理!");
                }
                else
                {
                    emit xSignaHelper.signalShowErrorMessageBar("日志清理失败!");
                }
            });


    // fileMenu->addSeparator();
    // action = configMenu->addAction("软件配置");
    // connect(action, &QAction::triggered, this, [this]() {
    //	AppCfgDialog dialog;
    //	dialog.exec();
    //	});

#define XTEST
#ifdef XTEST
    configMenu = menuBar->addMenu("测试");
    action = configMenu->addAction("快速打开文件");
    connect(action, &QAction::triggered, this,
            [this]()
            {
                if (AcqTaskManager::Instance().isAcquiring())
                {
                    ElaMessageBar::error(ElaMessageBarType::BottomRight, "错误",
                                         "当前正在进行采集，请等待当前采集结束!", 4000);
                    return;
                }
                QImage image = XImageHelper::openImageU16Raw(
                    "X:\\故宫\\data\\20251106150628936_海棠1106_步进CT扫描\\TWINDOW=3_WINDOW=0_TKEV=15_HKEV="
                    "60\\PROJECTION\\table=0_proj=0.raw",
                    1891, 496);
                _XGraphicsView->updateImage(image, true);
            });

    action = configMenu->addAction("测试QImage");
    connect(action, &QAction::triggered, this, [this]() { XImageHelper::testQImage(); });

    action = configMenu->addAction("测试opencv");
    connect(action, &QAction::triggered, this,
            [this]()
            {
                XImageHelper::testOpencv(
                    "X:\\故宫\\data\\20251106150628936_海棠1106_步进CT扫描\\TWINDOW=3_WINDOW=0_TKEV=15_HKEV="
                    "60\\PROJECTION\\table=0_proj=0.raw",
                    1891, 496);
            });

    action = configMenu->addAction("测试弹窗");
    connect(action, &QAction::triggered, this,
            [this]()
            {
                XElaDialog dialog("测试信息", XElaDialogType::INFO);
                dialog.showCentered();
            });
#endif  // XTEST
}

void MainWindow::initToolBar()
{
    // 工具栏
    ElaToolBar* toolBar = new ElaToolBar("工具栏", this);
    toolBar->setToolBarSpacing(3);
    toolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    toolBar->setIconSize(QSize(25, 25));

    // 打开图片文件
    toolButtonImport = new ElaToolButton(this);
    toolButtonImport->setElaIcon(ElaIconType::FileImport);
    toolButtonImport->setToolTip("打开图片文件");
    toolBar->addWidget(toolButtonImport);

    // 打开图片文件夹
    toolButtonOpenFolder = new ElaToolButton(this);
    toolButtonOpenFolder->setElaIcon(ElaIconType::FolderOpen);
    toolButtonOpenFolder->setToolTip("打开图片文件夹");
    toolBar->addWidget(toolButtonOpenFolder);

    // 打开图片文件夹
    toolButtonSaveFile = new ElaToolButton(this);
    toolButtonSaveFile->setElaIcon(ElaIconType::FileExport);
    toolButtonSaveFile->setToolTip("保存");
    toolBar->addWidget(toolButtonSaveFile);

    toolBar->addSeparator();

    // 缩小
    toolButtonZoomOut = new ElaToolButton(this);
    toolButtonZoomOut->setElaIcon(ElaIconType::MagnifyingGlassMinus);
    toolButtonZoomOut->setToolTip("缩小");
    toolBar->addWidget(toolButtonZoomOut);

    // 放大
    toolButtonZoomIn = new ElaToolButton(this);
    toolButtonZoomIn->setElaIcon(ElaIconType::MagnifyingGlassPlus);
    toolButtonZoomIn->setToolTip("放大");
    toolBar->addWidget(toolButtonZoomIn);

    // 缩放恢复
    toolButtonZoomRestore = new ElaToolButton(this);
    toolButtonZoomRestore->setElaIcon(ElaIconType::MagnifyingGlassArrowsRotate);
    toolButtonZoomRestore->setToolTip("恢复原始大小");
    toolBar->addWidget(toolButtonZoomRestore);

    toolBar->addSeparator();

    // DR拍照
    toolButtonDR = new ElaToolButton(this);
    toolButtonDR->setElaIcon(ElaIconType::Camera);
    toolButtonDR->setToolTip("DR拍照");
    toolBar->addWidget(toolButtonDR);

    // 实时DR
    toolButtonRealTimeDR = new ElaToolButton(this);
    toolButtonRealTimeDR->setElaIcon(ElaIconType::CameraMovie);
    toolButtonRealTimeDR->setToolTip("实时DR");
    toolBar->addWidget(toolButtonRealTimeDR);

    // 多张采集
    toolButtonDRMulti = new ElaToolButton(this);
    toolButtonDRMulti->setElaIcon(ElaIconType::CameraRetro);
    toolButtonDRMulti->setToolTip("多张采集");
    toolBar->addWidget(toolButtonDRMulti);

    // 停止实时DR
    toolButtonStopDR = new ElaToolButton(this);
    toolButtonStopDR->setElaIcon(ElaIconType::Stop);
    toolButtonStopDR->setToolTip("停止");
    toolBar->addWidget(toolButtonStopDR);

    connect(toolButtonImport, &ElaToolButton::clicked, _XGraphicsView, &XGraphicsView::openImage);
    connect(toolButtonOpenFolder, &ElaToolButton::clicked, _XGraphicsView, &XGraphicsView::openImageFolder);
    connect(toolButtonSaveFile, &ElaToolButton::clicked, _XGraphicsView, &XGraphicsView::saveImage);

    connect(toolButtonZoomOut, &ElaToolButton::clicked, _XGraphicsView, &XGraphicsView::zoomOut);
    connect(toolButtonZoomIn, &ElaToolButton::clicked, _XGraphicsView, &XGraphicsView::zoomIn);
    connect(toolButtonZoomRestore, &ElaToolButton::clicked, _XGraphicsView, &XGraphicsView::resetView);

    connect(toolButtonDR, &ElaToolButton::clicked, this, &MainWindow::onDROnceTimeBtnClicked);
    connect(toolButtonDRMulti, &ElaToolButton::clicked, this, &MainWindow::onDRMutliBtnClicked);
    connect(toolButtonRealTimeDR, &ElaToolButton::clicked, this, &MainWindow::onDRRealTimeBtnClicked);
    connect(toolButtonStopDR, &ElaToolButton::clicked, this, &MainWindow::onDRStopBtnClicked);

    this->addToolBar(Qt::TopToolBarArea, toolBar);
}

void MainWindow::connectToDevices()
{
    connectToXRay();
    connectToDet();
}

void MainWindow::connectToXRay()
{
    emit xSignaHelper.signalUpdateStatusInfo("开始连接射线源");
    bool bConnected = xRaySource.connectToSource("192.168.10.1", 10001);
    if (bConnected)
    {
        emit xSignaHelper.signalShowSuccessMessageBar("射线源已连接!");
    }
    else
    {
        emit xSignaHelper.signalShowErrorMessageBar("射线源连接失败!");
    }
}

void MainWindow::connectToDet()
{
#if DET_TYPE == DET_TYPE_VIRTUAL
    QThread::currentThread()->msleep(1000);
    emit xSignaHelper.signalUpdateStatusInfo("探测器已连接");
    emit xSignaHelper.signalShowSuccessMessageBar("探测器已连接");
#elif DET_TYPE == DET_TYPE_IRAY

    if (!DET.Initialize())
    {
        DET.DeInitialize();
        emit xSignaHelper.signalShowErrorMessageBar("探测器连接失败！");
    }
    else
    {
        emit xSignaHelper.signalUpdateStatusInfo("探测器已连接");
        emit xSignaHelper.signalShowSuccessMessageBar("探测器已连接");
        DET.StartQueryStatus();

        IRayDetectorWidget* pIRayDetectorWidget = new IRayDetectorWidget;
        pIRayDetectorWidget->showNormal();
    }
#endif  // DET_TYPE
}
