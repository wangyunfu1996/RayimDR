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
	void initMenuBar();
	void initToolBar();

	void connectToDevices();
	void connectToXRay();
	void connectToDet();

private:
	ElaContentDialog* _closeDialog{ nullptr };
	ElaText* _statusText{ nullptr };

	CommonConfigUI* _CommonConfigUI{ nullptr };

	XGraphicsView* _XGraphicsView{ nullptr };
	XImageAdjustTool* _XImageAdjustTool{ nullptr };

	ElaToolButton* toolButtonImport{ nullptr };
	ElaToolButton* toolButtonOpenFolder{ nullptr };
	ElaToolButton* toolButtonSaveFile{ nullptr };

	ElaToolButton* toolButtonZoomOut{ nullptr };
	ElaToolButton* toolButtonZoomIn{ nullptr };
	ElaToolButton* toolButtonZoomRestore{ nullptr };

	ElaToolButton* toolButtonDR{ nullptr };
	ElaToolButton* toolButtonRealTimeDR{ nullptr };
	ElaToolButton* toolButtonDRMulti{ nullptr };
	ElaToolButton* toolButtonStopDR{ nullptr };
};

