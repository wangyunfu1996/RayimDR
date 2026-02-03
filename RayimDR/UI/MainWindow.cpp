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

// ============================================================================
// Constructor / Destructor
// ============================================================================

MainWindow::MainWindow(QWidget* parent) : ElaWindow(parent)
{
    qDebug() << "[MainWindow] Initializing application";

    // Window configuration
    setupWindowProperties();

    // Create main UI components (includes layout setup)
    createUIComponents();

    // Initialize menu and toolbars
    initMenuBar();
    initToolBar();

    // Setup signal-slot connections
    setupConnections();

    // Connect to devices after UI is ready
    QTimer::singleShot(500, this, &MainWindow::connectToDevices);

    qDebug() << "[MainWindow] Initialization complete";
}

MainWindow::~MainWindow()
{
    qDebug() << "[MainWindow] Destroying main window";
}

// ============================================================================
// Window Setup Methods
// ============================================================================

void MainWindow::setupWindowProperties()
{
    setFocusPolicy(Qt::StrongFocus);
    setWindowIcon(QPixmap(":/Resource/Image/logo_32x32.ico"));
    setWindowTitle("锐影DR检测软件");

    // Theme and navigation settings
    setIsCentralStackedWidgetTransparent(true);
    eTheme->setThemeMode(ElaThemeType::Dark);
    setIsNavigationBarEnable(false);
    setUserInfoCardVisible(false);

    // Window button flags
    setWindowButtonFlag(ElaAppBarType::RouteBackButtonHint, false);
    setWindowButtonFlag(ElaAppBarType::RouteForwardButtonHint, false);
    setWindowButtonFlag(ElaAppBarType::NavigationButtonHint, false);
    setWindowButtonFlag(ElaAppBarType::ThemeChangeButtonHint, false);

    // Setup close dialog
    setIsDefaultClosed(false);
    _closeDialog = new ElaContentDialog(this);
    _closeDialog->setLeftButtonText("取消");
    _closeDialog->setMiddleButtonText("最小化");
    _closeDialog->setRightButtonText("确认");
}

void MainWindow::createUIComponents()
{
    // Main widget and components
    QWidget* mainWidget = new QWidget(this);
    _XGraphicsView = new XGraphicsView(mainWidget);
    _XImageAdjustTool = new XImageAdjustTool(mainWidget);
    _CommonConfigUI = new CommonConfigUI(mainWidget);

    // Setup layout
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

    // Add to central widget
    addPageNode("HOME", mainWidget, ElaIconType::House);

    // Status bar
    ElaStatusBar* statusBar = new ElaStatusBar(this);
    _statusText = new ElaText(QDateTime::currentDateTime().toString(), statusBar);
    _statusText->setTextPixelSize(15);
    statusBar->addWidget(_statusText, 1);
    setStatusBar(statusBar);
}

void MainWindow::setupConnections()
{
    qDebug() << "[MainWindow] Setting up signal-slot connections";

    // Close dialog connections
    connect(_closeDialog, &ElaContentDialog::rightButtonClicked, this, &MainWindow::onCloseDialogConfirmed);
    connect(this, &MainWindow::closeButtonClicked, this, &MainWindow::onCloseButtonClicked);

    // Global signal helper connections
    connect(&xSignaHelper, &XSignalsHelper::signalUpdateStatusInfo, this, &MainWindow::updateStatusText);
    connect(&xSignaHelper, &XSignalsHelper::signalShowErrorMessageBar, this, &MainWindow::onErrorMessageBar);
    connect(&xSignaHelper, &XSignalsHelper::signalShowSuccessMessageBar, this, &MainWindow::onSuccessMessageBar);

    // Graphics view and adjustment tool connections
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

    // Acquisition task manager connections
    connect(&AcqTaskManager::Instance(), &AcqTaskManager::signalAcqTaskStopped, this, &MainWindow::onAcqStopped);
    connect(&AcqTaskManager::Instance(), &AcqTaskManager::signalAcqTaskReceivedIdxChanged, this,
            &MainWindow::onAcqImageReceived);
    connect(&AcqTaskManager::Instance(), &AcqTaskManager::signalAcqStatusMsg, this, &MainWindow::onAcqStatusMessage);

    // Image helper connections
    connect(&XImageHelper::Instance(), &XImageHelper::signalOpenImageFolderProgressChanged, this,
            &MainWindow::onImageFolderProgressChanged);
}

// ============================================================================
// Close Event Handlers
// ============================================================================

void MainWindow::onCloseButtonClicked()
{
    qDebug() << "[MainWindow] Close button clicked";

    // Check if acquisition task is running
    if (AcqTaskManager::Instance().isAcquiring())
    {
        XElaDialog dialog("请先取消当前正在进行的采集任务!", XElaDialogType::ERR);
        dialog.showCentered();
        return;
    }

    _closeDialog->exec();
}

void MainWindow::onCloseDialogConfirmed()
{
    qDebug() << "[MainWindow] Close confirmed - shutting down";

    DET.StopQueryStatus();
    DET.DeInitialize();
    close();
}

// ============================================================================
// Signal Handlers
// ============================================================================

void MainWindow::onErrorMessageBar(const QString& msg)
{
    ElaMessageBar::error(ElaMessageBarType::TopRight, "错误", msg, 4000);
}

void MainWindow::onSuccessMessageBar(const QString& msg)
{
    ElaMessageBar::success(ElaMessageBarType::TopRight, "成功", msg, 4000);
}

void MainWindow::onAcqStatusMessage(const QString& msg, int errorCode)
{
    updateStatusText(msg);
    if (errorCode == 1)
    {
        ElaMessageBar::error(ElaMessageBarType::TopRight, "错误", msg, 4000);
    }
}

void MainWindow::onImageFolderProgressChanged(int progress)
{
    updateStatusText(QString("数据读取中，处理进度: %1%").arg(progress));
}

void MainWindow::onXRayStopRequested()
{
    qDebug() << "[MainWindow] X-ray stop requested";

    XElaDialog dialog("采集结束，是否关闭射线源？", XElaDialogType::ASK);
    if (dialog.showCentered() == QDialog::Accepted)
    {
        IXS120BP120P366::Instance().stopXRay();
    }
}

// ============================================================================
// Status Display
// ============================================================================

void MainWindow::updateStatusText(const QString& msg)
{
    qDebug() << "[Status]" << msg;
    _statusText->setText(msg);
}

// ============================================================================
// Menu Action Handlers
// ============================================================================

void MainWindow::onMenuFileOpen()
{
    qDebug() << "[MainWindow] Menu: Open image file";
    _XGraphicsView->openImage();
}

void MainWindow::onMenuFileOpenFolder()
{
    qDebug() << "[MainWindow] Menu: Open image folder";
    _XGraphicsView->openImageFolder();
}

void MainWindow::onMenuFileSave()
{
    qDebug() << "[MainWindow] Menu: Save image";
    _XGraphicsView->saveImage();
}

void MainWindow::onMenuFileExit()
{
    qDebug() << "[MainWindow] Menu: Exit application";
    _closeDialog->exec();
}

void MainWindow::onMenuXRayConfig()
{
    qDebug() << "[MainWindow] Menu: X-ray configuration";
    XRayCfgDialog dialog;
    dialog.exec();
}

void MainWindow::onMenuDetectorConfig()
{
    qDebug() << "[MainWindow] Menu: Detector configuration";
    IRayDetectorWidget* widget = new IRayDetectorWidget;
    widget->setAttribute(Qt::WA_DeleteOnClose);
    widget->showNormal();
}

void MainWindow::onMenuDetectorCalibration()
{
    qDebug() << "[MainWindow] Menu: Detector calibration";

    if (!DET.Initialized())
    {
        emit xSignaHelper.signalShowErrorMessageBar("探测器未连接!");
        return;
    }

    CreateCorrectTemplateDlg dialog;
    dialog.exec();
}

void MainWindow::onMenuCleanupLogs()
{
    qDebug() << "[MainWindow] Menu: Cleanup logs";

    if (QtLogger::cleanupLogs())
    {
        emit xSignaHelper.signalShowSuccessMessageBar("日志已清理!");
    }
    else
    {
        emit xSignaHelper.signalShowErrorMessageBar("日志清理失败!");
    }
}

// ============================================================================
// Menu and Toolbar Initialization
// ============================================================================

void MainWindow::initMenuBar()
{
    qDebug() << "[MainWindow] Initializing menu bar";

    ElaMenuBar* menuBar = new ElaMenuBar(this);
    setMenuBar(menuBar);

    // File menu
    ElaMenu* fileMenu = menuBar->addMenu("文件");
    connect(fileMenu->addAction("打开图像文件"), &QAction::triggered, this, &MainWindow::onMenuFileOpen);
    connect(fileMenu->addAction("打开图像文件夹"), &QAction::triggered, this, &MainWindow::onMenuFileOpenFolder);
    connect(fileMenu->addAction("保存"), &QAction::triggered, this, &MainWindow::onMenuFileSave);
    fileMenu->addSeparator();
    connect(fileMenu->addAction("退出"), &QAction::triggered, this, &MainWindow::onMenuFileExit);

    // Settings menu
    ElaMenu* configMenu = menuBar->addMenu("设置");
    connect(configMenu->addAction("射线源设置"), &QAction::triggered, this, &MainWindow::onMenuXRayConfig);
    connect(configMenu->addAction("探测器设置"), &QAction::triggered, this, &MainWindow::onMenuDetectorConfig);
    connect(configMenu->addAction("探测器校正"), &QAction::triggered, this, &MainWindow::onMenuDetectorCalibration);
    connect(configMenu->addAction("清理日志"), &QAction::triggered, this, &MainWindow::onMenuCleanupLogs);

    qDebug() << "[MainWindow] Menu bar initialized";
}

void MainWindow::initToolBar()
{
    qDebug() << "[MainWindow] Initializing toolbar";

    ElaToolBar* toolBar = new ElaToolBar("工具栏", this);
    toolBar->setToolBarSpacing(3);
    toolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    toolBar->setIconSize(QSize(25, 25));

    // File operations
    toolButtonImport = new ElaToolButton(this);
    toolButtonImport->setElaIcon(ElaIconType::FileImport);
    toolButtonImport->setToolTip("打开图片文件");
    toolBar->addWidget(toolButtonImport);

    toolButtonOpenFolder = new ElaToolButton(this);
    toolButtonOpenFolder->setElaIcon(ElaIconType::FolderOpen);
    toolButtonOpenFolder->setToolTip("打开图片文件夹");
    toolBar->addWidget(toolButtonOpenFolder);

    toolButtonSaveFile = new ElaToolButton(this);
    toolButtonSaveFile->setElaIcon(ElaIconType::FileExport);
    toolButtonSaveFile->setToolTip("保存");
    toolBar->addWidget(toolButtonSaveFile);

    toolBar->addSeparator();

    // Zoom operations
    toolButtonZoomOut = new ElaToolButton(this);
    toolButtonZoomOut->setElaIcon(ElaIconType::MagnifyingGlassMinus);
    toolButtonZoomOut->setToolTip("缩小");
    toolBar->addWidget(toolButtonZoomOut);

    toolButtonZoomIn = new ElaToolButton(this);
    toolButtonZoomIn->setElaIcon(ElaIconType::MagnifyingGlassPlus);
    toolButtonZoomIn->setToolTip("放大");
    toolBar->addWidget(toolButtonZoomIn);

    toolButtonZoomRestore = new ElaToolButton(this);
    toolButtonZoomRestore->setElaIcon(ElaIconType::MagnifyingGlassArrowsRotate);
    toolButtonZoomRestore->setToolTip("恢复原始大小");
    toolBar->addWidget(toolButtonZoomRestore);

    toolBar->addSeparator();

    // Acquisition operations
    toolButtonDR = new ElaToolButton(this);
    toolButtonDR->setElaIcon(ElaIconType::Camera);
    toolButtonDR->setToolTip("DR拍照");
    toolBar->addWidget(toolButtonDR);

    toolButtonRealTimeDR = new ElaToolButton(this);
    toolButtonRealTimeDR->setElaIcon(ElaIconType::CameraMovie);
    toolButtonRealTimeDR->setToolTip("实时DR");
    toolBar->addWidget(toolButtonRealTimeDR);

    toolButtonDRMulti = new ElaToolButton(this);
    toolButtonDRMulti->setElaIcon(ElaIconType::CameraRetro);
    toolButtonDRMulti->setToolTip("多张采集");
    toolBar->addWidget(toolButtonDRMulti);

    toolButtonStopDR = new ElaToolButton(this);
    toolButtonStopDR->setElaIcon(ElaIconType::Stop);
    toolButtonStopDR->setToolTip("停止");
    toolBar->addWidget(toolButtonStopDR);

    // Connect toolbar buttons
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

    addToolBar(Qt::TopToolBarArea, toolBar);

    qDebug() << "[MainWindow] Toolbar initialized";
}

// ============================================================================
// Acquisition Workflow Methods
// ============================================================================

void MainWindow::onAcqStarted(const AcqCondition& acqCond)
{
    qDebug() << "[MainWindow] Acquisition started -" << acqCond;

    // Disable file operations during acquisition
    toolButtonImport->setEnabled(false);
    toolButtonOpenFolder->setEnabled(false);
    toolButtonSaveFile->setEnabled(false);

    // Disable acquisition buttons
    toolButtonDR->setEnabled(false);
    toolButtonDRMulti->setEnabled(false);
    toolButtonRealTimeDR->setEnabled(false);

    // Clear previous data
    _XGraphicsView->clearROIRect();
    _XGraphicsView->clearCurrentImageList();

    // Configure image index slider based on acquisition type
    if (acqCond.frame == 1 || acqCond.frame == INT_MAX)
    {
        // Single frame or continuous acquisition: disable index slider
        _XImageAdjustTool->updateIdxRange(0);
    }
    else
    {
        // Multi-frame acquisition: enable index slider
        _XImageAdjustTool->updateIdxRange(acqCond.frame);
    }

    updateStatusText("开始采集");
}

void MainWindow::onAcqStopped()
{
    qDebug() << "[MainWindow] Acquisition stopped";

    // Re-enable file operations
    toolButtonImport->setEnabled(true);
    toolButtonOpenFolder->setEnabled(true);
    toolButtonSaveFile->setEnabled(true);

    // Re-enable acquisition buttons
    toolButtonDR->setEnabled(true);
    toolButtonDRMulti->setEnabled(true);
    toolButtonRealTimeDR->setEnabled(true);

    // Check if X-ray source should be turned off
    onXRayStopRequested();
}

void MainWindow::onAcqImageReceived(AcqCondition condition, int receivedIdx)
{
    qDebug() << "[MainWindow] Image received - Frame" << receivedIdx << "Condition:" << condition;

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
    qDebug() << "[MainWindow] DR single shot requested";

    if (!_CommonConfigUI->checkInputValid())
    {
        qDebug() << "[MainWindow] Input validation failed";
        return;
    }

    AcqCondition acqCond = _CommonConfigUI->getAcqCondition();
    acqCond.acqType = AcqType::DR;
    acqCond.frame = 1;

    AcqTaskManager::Instance().updateAcqCond(acqCond);
    AcqTaskManager::Instance().startAcq();
    onAcqStarted(acqCond);
}

void MainWindow::onDRMutliBtnClicked()
{
    qDebug() << "[MainWindow] DR multi-frame acquisition requested";

    MultiAcqCfgDialog dialog;
    if (dialog.exec() != QDialog::Accepted)
    {
        return;
    }

    AcqCondition acqCond = _CommonConfigUI->getAcqCondition();
    acqCond.acqType = AcqType::DR;

    if (!dialog.getCfg(acqCond.frame, acqCond.saveToFiles, acqCond.savePath, acqCond.saveType))
    {
        emit xSignaHelper.signalShowErrorMessageBar("获取多帧采集参数错误！");
        qDebug() << "[MainWindow] Failed to get multi-frame config:" << acqCond;
        return;
    }

    if (!_CommonConfigUI->checkInputValid())
    {
        qDebug() << "[MainWindow] Input validation failed";
        return;
    }

    AcqTaskManager::Instance().updateAcqCond(acqCond);
    AcqTaskManager::Instance().startAcq();
    onAcqStarted(acqCond);
}

void MainWindow::onDRRealTimeBtnClicked()
{
    qDebug() << "[MainWindow] Real-time DR requested";

    if (!_CommonConfigUI->checkInputValid())
    {
        qDebug() << "[MainWindow] Input validation failed";
        return;
    }

    AcqCondition acqCond = _CommonConfigUI->getAcqCondition();
    acqCond.acqType = AcqType::DR;
    acqCond.frame = INT_MAX;

    AcqTaskManager::Instance().updateAcqCond(acqCond);
    AcqTaskManager::Instance().startAcq();
    onAcqStarted(acqCond);
}

void MainWindow::onDRStopBtnClicked()
{
    qDebug() << "[MainWindow] DR stop requested";
    AcqTaskManager::Instance().stopAcq();
}

// ============================================================================
// Device Connection Methods
// ============================================================================

void MainWindow::connectToDevices()
{
    qDebug() << "[MainWindow] Connecting to devices";
    connectToXRay();
    connectToDet();
}

void MainWindow::connectToXRay()
{
    qDebug() << "[MainWindow] Connecting to X-ray source";
    emit xSignaHelper.signalUpdateStatusInfo("开始连接射线源");

    bool connected = xRaySource.connectToSource(XRAY_DEVICE_IP.c_str(), XRAY_DEVICE_PORT);
    if (connected)
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
    qDebug() << "[MainWindow] Connecting to detector";

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
    }
#endif  // DET_TYPE
}
