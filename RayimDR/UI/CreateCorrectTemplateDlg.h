#pragma once

#include "ElaDialog.h"
#include "ui_CreateCorrectTemplateDlg.h"
#include <QSharedPointer>

class CreateCorrectTemplateDlg : public ElaDialog
{
    Q_OBJECT

public:
    CreateCorrectTemplateDlg(QWidget* parent = nullptr);
    ~CreateCorrectTemplateDlg();

private:
// Calibration workflow methods
void Abort();
void Offset();   // Dark field calibration
void Gain();     // Bright field calibration
void Defect();   // Defect calibration

// UI helper methods
void ShowTips(const QString& msg);
void ModifyGainVoltageCurrent(int voltage, int current);
void ModifyDefectVoltageCurrent(int voltage, int current);
void ModifyMode(int modeIdx);

// Initialization helper methods
void initializeWindow();
void initializeGraphicsViews();
void initializeControls();
void connectSignals();
void initializeDetectorMode();

// X-ray source control helpers
void startXRaySource(int voltage, int current);
void stopXRaySource();
void adjustCurrentUntilTargetGray(int& currentValue, int targetGray, int ptst);

private:
    void onOffsetImageSelected(int nTotal, int nValid);
    void onGainAcqImageReceived(QSharedPointer<QImage> image, int idx, int grayValue);
    void onGainImageSelected(int nTotal, int nValid);
    void onDefectAcqImageReceived(QSharedPointer<QImage> image, int idx, int grayValue);
    void onDefectGroupChanged(int groupIdx, int nTotalGroup);
    void onDefectImageSelected(int nTotal, int nValid);

signals:
    void signalTipsChanged(const QString& msg);
    void signalGainVoltageCurrentChanged(int voltage, int current);
    void signalDefectVoltageCurrentChanged(int voltage, int current);
    void signalDefectGroupChanged(int groupIdx, int nTotalGroup);

private:
    Ui::CreateCorrectTemplateDlgClass ui;
    int nCurrentGray{0};
    class QGraphicsPixmapItem* gainPixmapItem{nullptr};
    class QGraphicsPixmapItem* defectPixmapItem{nullptr};
};
