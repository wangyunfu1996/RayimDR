#pragma once

#include <QMainWindow>

#include "ElaWindow.h"

#include "Components/XGlobal.h"

class ElaToolButton;
class ElaContentDialog;
class ElaText;

class CommonConfigUI;

class XGraphicsView;
class XImageAdjustTool;

class MainWindow : public ElaWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    void updateStatusText(const QString& msg);
    void onAcqStarted(const AcqCondition& acqCond);
    void onAcqStopped();
    void onAcqImageReceived(AcqCondition condition, int receivedIdx);

    void onDROnceTimeBtnClicked();
    void onDRMutliBtnClicked();
    void onDRRealTimeBtnClicked();
    void onDRStopBtnClicked();

private:
    // Initialization methods
    void setupWindowProperties();
    void createUIComponents();
    void initMenuBar();
    void initToolBar();
    void setupConnections();

    // Device connection methods
    void connectToDevices();
    void connectToXRay();
    void connectToDet();

    // Menu action handlers
    void onMenuFileOpen();
    void onMenuFileOpenFolder();
    void onMenuFileSave();
    void onMenuFileExit();
    void onMenuXRayConfig();
    void onMenuDetectorConfig();
    void onMenuDetectorCalibration();
    void onMenuAppCfg();
    void onMenuCleanupLogs();

    // Close event handlers
    void onCloseButtonClicked();
    void onCloseDialogConfirmed();

    // Signal handlers
    void onErrorMessageBar(const QString& msg, int displayMsec);
    void onSuccessMessageBar(const QString& msg, int displayMsec);
    void onAcqErr(const QString& msg);
    void onAcqProgressChanged(const QString& msg);
    void onImageFolderProgressChanged(int progress);
    void onXRayStopRequested();

private:
    ElaContentDialog* _closeDialog{nullptr};
    ElaText* _statusText{nullptr};

    CommonConfigUI* _CommonConfigUI{nullptr};

    XGraphicsView* _XGraphicsView{nullptr};
    XImageAdjustTool* _XImageAdjustTool{nullptr};

    ElaToolButton* toolButtonImport{nullptr};
    ElaToolButton* toolButtonOpenFolder{nullptr};
    ElaToolButton* toolButtonSaveFile{nullptr};

    ElaToolButton* toolButtonZoomOut{nullptr};
    ElaToolButton* toolButtonZoomIn{nullptr};
    ElaToolButton* toolButtonZoomRestore{nullptr};

    ElaToolButton* toolButtonDR{nullptr};
    ElaToolButton* toolButtonRealTimeDR{nullptr};
    ElaToolButton* toolButtonDRMulti{nullptr};
    ElaToolButton* toolButtonStopDR{nullptr};
};
