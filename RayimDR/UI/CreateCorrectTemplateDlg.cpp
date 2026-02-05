#include "CreateCorrectTemplateDlg.h"

#include <qgraphicsscene.h>
#include <qgraphicsitem.h>
#include <QtConcurrent/QtConcurrent>
#include <QFutureWatcher>

#include "IRayDetector/NDT1717MA.h"
#include "VJXRAY/IXS120BP120P366.h"
#include "ImageRender/XImageHelper.h"

CreateCorrectTemplateDlg::CreateCorrectTemplateDlg(QWidget* parent) : ElaDialog(parent)
{
    ui.setupUi(this);

    // Window setup
    initializeWindow();

    // Graphics views setup
    initializeGraphicsViews();

    // UI controls setup
    initializeControls();

    // Signal connections
    connectSignals();

    // Initialize detector mode
    initializeDetectorMode();

    // Set initial page
    ui.stackedWidget->setCurrentIndex(0);
}

CreateCorrectTemplateDlg::~CreateCorrectTemplateDlg() {}

// ============================================================================
// Initialization Helper Methods
// ============================================================================

void CreateCorrectTemplateDlg::initializeWindow()
{
    setWindowTitle("探测器校正");
    setWindowButtonFlag(ElaAppBarType::StayTopButtonHint, false);
    setWindowButtonFlag(ElaAppBarType::MinimizeButtonHint, false);
    setWindowButtonFlag(ElaAppBarType::MaximizeButtonHint, false);
    resize(800, 400);
    moveToCenter();
}

void CreateCorrectTemplateDlg::initializeGraphicsViews()
{
    // Gain image view
    auto gainScene = new QGraphicsScene(ui.graphicsView_GainImageView);
    gainPixmapItem = new QGraphicsPixmapItem();
    gainScene->addItem(gainPixmapItem);
    ui.graphicsView_GainImageView->setScene(gainScene);

    // Defect image view
    auto defectScene = new QGraphicsScene(ui.graphicsView_DefectImageView);
    defectPixmapItem = new QGraphicsPixmapItem();
    defectScene->addItem(defectPixmapItem);
    ui.graphicsView_DefectImageView->setScene(defectScene);
}

void CreateCorrectTemplateDlg::initializeControls()
{
    // Voltage and current ranges
    constexpr int MIN_VOLTAGE = 30;
    constexpr int MAX_VOLTAGE = 120;

    // uA
    constexpr int MIN_CURRENT = 200;
    constexpr int MAX_CURRENT = 1000;

    // Gain controls
    ui.spinBox_GainVoltage->setRange(MIN_VOLTAGE, MAX_VOLTAGE);
    ui.spinBox_GainCurrent->setRange(MIN_CURRENT, MAX_CURRENT);

    // Defect controls
    ui.spinBox_DefectVoltage->setRange(0, MAX_VOLTAGE);
    ui.spinBox_DefectCurrent->setRange(MIN_CURRENT, MAX_CURRENT);

    // Progress displays (read-only)
    ui.lineEdit_OffsetProgress->setEnabled(false);
    ui.lineEdit_DefectGroup->setEnabled(false);
    ui.lineEdit_DefectExceptedGray->setEnabled(false);
    ui.lineEdit_DefectCurrentGray->setEnabled(false);
    ui.lineEdit_DefectProgress->setEnabled(false);

    // Buttons initial state
    ui.pushButton_OffsetGenerationStart->setEnabled(true);
    ui.pushButton_OffsetGenerationAbort->setEnabled(false);
    ui.pushButton_DefectAbort->setEnabled(false);

    // Disable defect voltage/current spinboxes (auto-adjusted)
    ui.spinBox_DefectVoltage->setEnabled(false);
    ui.spinBox_DefectCurrent->setEnabled(false);

    // Stage label
    ui.label_Stage->setEnabled(false);

    // Detector mode combo box
    ui.comboBox_mode->addItems({"1x1", "2x2", "3x3", "4x4"});
}

void CreateCorrectTemplateDlg::connectSignals()
{
    // Navigation buttons
    connect(ui.pushButton_ToGain, &QPushButton::clicked, this,
            [this]()
            {
                ui.label_Tips->clear();
                ui.stackedWidget->setCurrentIndex(1);
                ui.pushButton_GainAbort->setEnabled(false);
                ui.pushButton_Gain->setEnabled(true);
                ui.spinBox_GainCurrent->setEnabled(false);
                ui.spinBox_GainVoltage->setEnabled(false);
            });

    connect(ui.pushButton_ToOffset, &QPushButton::clicked, this, [this]() { ui.stackedWidget->setCurrentIndex(0); });

    connect(ui.pushButton_ToDefect, &QPushButton::clicked, this, [this]() { ui.stackedWidget->setCurrentIndex(2); });

    connect(ui.pushButton_ReturnToGain, &QPushButton::clicked, this,
            [this]() { ui.stackedWidget->setCurrentIndex(1); });

    connect(ui.pushButton_Done, &QPushButton::clicked, this, [this]() { accept(); });

    // Calibration action buttons
    connect(ui.pushButton_OffsetGenerationStart, &QPushButton::clicked, this, &CreateCorrectTemplateDlg::Offset);
    connect(ui.pushButton_OffsetGenerationAbort, &QPushButton::clicked, this, &CreateCorrectTemplateDlg::Abort);

    connect(ui.pushButton_Gain, &QPushButton::clicked, this, &CreateCorrectTemplateDlg::Gain);
    connect(ui.pushButton_GainAbort, &QPushButton::clicked, this, &CreateCorrectTemplateDlg::Abort);

    connect(ui.pushButton_Defect, &QPushButton::clicked, this, &CreateCorrectTemplateDlg::Defect);
    connect(ui.pushButton_DefectAbort, &QPushButton::clicked, this, &CreateCorrectTemplateDlg::Abort);

    // Tips display
    connect(this, &CreateCorrectTemplateDlg::signalTipsChanged, this, &CreateCorrectTemplateDlg::ShowTips,
            Qt::UniqueConnection);

    // Stage label update
    connect(ui.stackedWidget, &QStackedWidget::currentChanged, this,
            [this](int idx)
            {
                const QStringList stages = {"当前流程：暗场校正", "当前流程：亮场校正", "当前流程：缺陷校正"};
                if (idx >= 0 && idx < stages.size())
                {
                    ui.label_Stage->setText(stages[idx]);
                }
            });

    // Detector mode change
    connect(ui.comboBox_mode, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &CreateCorrectTemplateDlg::ModifyMode);
}

void CreateCorrectTemplateDlg::initializeDetectorMode()
{
    // Get current detector mode
    std::string mode;
    DET.GetMode(mode);

    // Map mode string to combo box index
    const QMap<QString, QString> modeMap = {{"Mode5", "1x1"}, {"Mode6", "2x2"}, {"Mode7", "3x3"}, {"Mode8", "4x4"}};

    QString currentMode = modeMap.value(QString::fromStdString(mode));
    if (!currentMode.isEmpty())
    {
        ui.comboBox_mode->setCurrentText(currentMode);
    }
}

// ============================================================================
// X-ray Source Control Helpers
// ============================================================================

void CreateCorrectTemplateDlg::startXRaySource(int voltage, int current)
{
    qDebug() << "[X-Ray] Starting X-ray source - Voltage:" << voltage << "kV, Current:" << current << "uA";

    IXS120BP120P366::Instance().setVoltage(voltage);
    IXS120BP120P366::Instance().setCurrent(current);
    IXS120BP120P366::Instance().startXRay();

    int ptst = IXS120BP120P366::Instance().getPTST();
    qDebug() << "[X-Ray] PTST (warmup time):" << ptst << "seconds";

    QThread::msleep(ptst * 1000 + 3000);
    emit signalTipsChanged("射线源已开启");
}

void CreateCorrectTemplateDlg::stopXRaySource()
{
    qDebug() << "[X-Ray] Stopping X-ray source";
    IXS120BP120P366::Instance().stopXRay();
    emit signalTipsChanged("射线源已关闭");
}

void CreateCorrectTemplateDlg::adjustCurrentUntilTargetGray(int& currentValue, int targetGray, int ptst)
{
    constexpr int MAX_CURRENT = 1000;
    constexpr int CURRENT_STEP = 100;
    constexpr int ADJUSTMENT_DELAY = 3000;  // milliseconds

    while (nCurrentGray < targetGray)
    {
        if (currentValue >= MAX_CURRENT)
        {
            qDebug() << "[X-Ray] Maximum current reached:" << MAX_CURRENT << "uA";
            break;
        }

        currentValue += CURRENT_STEP;
        qDebug() << "[Adjustment] Target gray:" << targetGray << "Current gray:" << nCurrentGray
                 << "New current:" << currentValue << "uA";

        IXS120BP120P366::Instance().setCurrent(currentValue);
        QThread::msleep(ptst * 1000 + ADJUSTMENT_DELAY);
    }
}

// ============================================================================
// Calibration Workflow Methods
// ============================================================================

void CreateCorrectTemplateDlg::Abort()
{
    DET.Abort();
}

void CreateCorrectTemplateDlg::Offset()
{
    qDebug() << "[Offset] Starting dark field calibration";

    ui.pushButton_OffsetGenerationStart->setEnabled(false);
    ui.pushButton_OffsetGenerationAbort->setEnabled(true);
    ui.lineEdit_OffsetProgress->clear();

    connect(&DET, &NDT1717MA::signalOffsetImageSelected, this, &CreateCorrectTemplateDlg::onOffsetImageSelected,
            Qt::UniqueConnection);

    auto future = QtConcurrent::run([this]() { return DET.OffsetGeneration(); });

    auto* watcher = new QFutureWatcher<bool>(this);
    connect(watcher, &QFutureWatcher<bool>::finished, this,
            [this, watcher]()
            {
                ui.pushButton_OffsetGenerationStart->setEnabled(true);
                ui.pushButton_OffsetGenerationAbort->setEnabled(false);

                bool result = watcher->future().result();
                qDebug() << "[Offset] Calibration completed. Result:" << result;

                if (result)
                {
                    emit signalTipsChanged("暗场校正完成，请进行下一步校正");
                }
                else
                {
                    emit signalTipsChanged("暗场校正失败");
                }

                disconnect(&DET, &NDT1717MA::signalOffsetImageSelected, this,
                           &CreateCorrectTemplateDlg::onOffsetImageSelected);
                watcher->deleteLater();
            });
    watcher->setFuture(future);
}

void CreateCorrectTemplateDlg::Gain()
{
    qDebug() << "[Gain] Starting bright field calibration";

    // Setup signal connections
    connect(this, &CreateCorrectTemplateDlg::signalGainVoltageCurrentChanged, this,
            &CreateCorrectTemplateDlg::ModifyGainVoltageCurrent, Qt::UniqueConnection);
    connect(&DET, &NDT1717MA::signalAcqImageReceived, this, &CreateCorrectTemplateDlg::onGainAcqImageReceived,
            Qt::UniqueConnection);
    connect(&DET, &NDT1717MA::signalGainImageSelected, this, &CreateCorrectTemplateDlg::onGainImageSelected,
            Qt::UniqueConnection);

    // Update UI state
    ui.pushButton_Gain->setEnabled(false);
    ui.pushButton_GainAbort->setEnabled(true);
    ui.label_Tips->clear();

    // Execute calibration in background thread
    auto future = QtConcurrent::run(
        [this]()
        {
            nCurrentGray = 0;

            // Step 1: Initialize
            qDebug() << "[Gain] Step 1: Initializing";
            emit signalTipsChanged("正在执行初始化");
            if (!DET.GainInit())
            {
                emit signalTipsChanged("初始化失败！");
                return false;
            }

            // Step 2: Start X-ray source
            int voltage = DET.nGainVoltage;
            int current = DET.nGainBeginCurrent;
            int ptst = 0;

            qDebug() << "[Gain] Step 2: Starting X-ray source - Voltage:" << voltage << "kV, Current:" << current
                     << "uA";
            emit signalTipsChanged("正在等待射线源开启");

            IXS120BP120P366::Instance().setVoltage(voltage);
            IXS120BP120P366::Instance().setCurrent(current);
            IXS120BP120P366::Instance().startXRay();

            ptst = IXS120BP120P366::Instance().getPTST();
            qDebug() << "[Gain] X-ray warmup time (PTST):" << ptst << "seconds";
            QThread::msleep(ptst * 1000 + 3000);

            emit signalTipsChanged("射线源已开启");

            // Step 3: Start acquisition
            qDebug() << "[Gain] Step 3: Starting acquisition";
            emit signalTipsChanged("正在自动调整电压电流，并采集亮场");

            if (!DET.GainStartAcq())
            {
                emit signalTipsChanged("开启采集失败！");
                return false;
            }

            // Wait for first data
            QThread::msleep(2000);
            emit signalGainVoltageCurrentChanged(voltage, current);

            // Step 4: Adjust current until target gray value
            qDebug() << "[Gain] Step 4: Adjusting current to reach target gray value";
            while (nCurrentGray < DET.nGainExpectedGray)
            {
                if (DET.CurrentTransaction() != 1)
                {
                    qDebug() << "[Gain] Calibration aborted by user";
                    return false;
                }

                qDebug() << "[Gain] Gray level - Current:" << nCurrentGray << "Target:" << DET.nGainExpectedGray
                         << "Current:" << current << "uA";

                if (current >= 1000)
                {
                    qDebug() << "[Gain] Maximum current reached: 1000 uA";
                    break;
                }

                current += 100;
                emit signalGainVoltageCurrentChanged(voltage, current);
                IXS120BP120P366::Instance().setCurrent(current);
                QThread::msleep(ptst * 1000 + 3000);
            }

            // Step 5: Select images
            if (DET.CurrentTransaction() != 1)
            {
                return false;
            }

            qDebug() << "[Gain] Step 5: Selecting images";
            if (!DET.GainSelectAll().get())
            {
                emit signalTipsChanged("获取合适的图像失败！");
                return false;
            }
            emit signalTipsChanged("亮场采集结束");

            // Step 6: Generate gain correction
            qDebug() << "[Gain] Step 6: Generating gain correction";
            emit signalTipsChanged("正在执行计算");
            if (!DET.GainGeneration())
            {
                emit signalTipsChanged("计算失败！");
                return false;
            }

            qDebug() << "[Gain] Calibration completed successfully";
            return true;
        });

    // Setup watcher for completion
    auto* watcher = new QFutureWatcher<bool>(this);
    connect(watcher, &QFutureWatcher<bool>::finished, this,
            [this, watcher]()
            {
                ui.pushButton_Gain->setEnabled(true);
                ui.pushButton_GainAbort->setEnabled(false);

                stopXRaySource();

                bool result = watcher->future().result();
                qDebug() << "[Gain] Workflow finished. Result:" << result;

                if (result)
                {
                    emit signalTipsChanged("亮场校正完成，请进行下一步校正");
                }
                else
                {
                    emit signalTipsChanged("亮场校正流程失败！");
                }

                disconnect(&DET, &NDT1717MA::signalAcqImageReceived, this,
                           &CreateCorrectTemplateDlg::onGainAcqImageReceived);
                watcher->deleteLater();
            });
    watcher->setFuture(future);
}

void CreateCorrectTemplateDlg::Defect()
{
    qDebug() << "[Defect] Starting defect calibration";

    // Setup signal connections
    connect(this, &CreateCorrectTemplateDlg::signalDefectVoltageCurrentChanged, this,
            &CreateCorrectTemplateDlg::ModifyDefectVoltageCurrent, Qt::UniqueConnection);
    connect(this, &CreateCorrectTemplateDlg::signalDefectGroupChanged, this,
            &CreateCorrectTemplateDlg::onDefectGroupChanged, Qt::UniqueConnection);
    connect(&DET, &NDT1717MA::signalAcqImageReceived, this, &CreateCorrectTemplateDlg::onDefectAcqImageReceived,
            Qt::UniqueConnection);
    connect(&DET, &NDT1717MA::signalDefectImageSelected, this, &CreateCorrectTemplateDlg::onDefectImageSelected,
            Qt::UniqueConnection);

    // Update UI state
    ui.pushButton_Defect->setEnabled(false);
    ui.pushButton_DefectAbort->setEnabled(true);
    ui.label_Tips->clear();

    // Execute calibration in background thread
    auto future = QtConcurrent::run(
        [this]()
        {
            nCurrentGray = 0;

            // Step 1: Initialize
            qDebug() << "[Defect] Step 1: Initializing";
            emit signalTipsChanged("正在执行初始化");

            if (!DET.DefectInit())
            {
                emit signalTipsChanged("初始化失败！");
                return false;
            }

            // Process each group
            for (int groupIdx = 0; groupIdx < DET.nTotalGroup; groupIdx++)
            {
                qDebug() << "[Defect] Processing group" << (groupIdx + 1) << "/" << DET.nTotalGroup;
                emit signalDefectGroupChanged(groupIdx, DET.nTotalGroup);

                int voltage = DET.nDefectVoltages[groupIdx];
                int current = DET.nDefectBeginCurrents[groupIdx];
                int targetGray = DET.nDefectExpectedGrays[groupIdx];
                int ptst = 0;

                // Step 2: Start X-ray source for this group
                qDebug() << "[Defect] Group" << (groupIdx + 1) << "- Starting X-ray source"
                         << "Voltage:" << voltage << "kV, Current:" << current << "uA, Target gray:" << targetGray;

                emit signalTipsChanged("等待射线源开启");

                IXS120BP120P366::Instance().setVoltage(voltage);
                IXS120BP120P366::Instance().setCurrent(current);
                IXS120BP120P366::Instance().startXRay();

                ptst = IXS120BP120P366::Instance().getPTST();
                qDebug() << "[Defect] Group" << (groupIdx + 1) << "- Warmup time (PTST):" << ptst << "seconds";
                QThread::msleep(ptst * 1000 + 3000);

                emit signalTipsChanged("射线源已开启");

                // Step 3: Start acquisition
                qDebug() << "[Defect] Group" << (groupIdx + 1) << "- Starting acquisition";
                emit signalTipsChanged("正在自动调整电压电流，并采集亮场");

                if (!DET.DefectStartAcq())
                {
                    emit signalTipsChanged("开启采集失败！");
                    return false;
                }

                // Wait for first data
                QThread::msleep(2000);

                // Step 4: Adjust current until target gray value
                qDebug() << "[Defect] Group" << (groupIdx + 1) << "- Adjusting current to reach target gray";

                while (nCurrentGray < targetGray)
                {
                    if (DET.CurrentTransaction() != 2)
                    {
                        qDebug() << "[Defect] Calibration aborted by user";
                        return false;
                    }

                    qDebug() << "[Defect] Group" << (groupIdx + 1) << "- Gray level - Current:" << nCurrentGray
                             << "Target:" << targetGray << "Current:" << current << "uA";

                    if (current >= 1000)
                    {
                        qDebug() << "[Defect] Maximum current reached: 1000 uA";
                        break;
                    }

                    current += 100;
                    emit signalDefectVoltageCurrentChanged(voltage, current);
                    IXS120BP120P366::Instance().setCurrent(current);
                    QThread::msleep(2000);
                }

                if (DET.CurrentTransaction() != 2)
                {
                    return false;
                }

                // Step 5: Select images
                qDebug() << "[Defect] Group" << (groupIdx + 1) << "- Selecting images";
                if (!DET.DefectSelectAll(groupIdx))
                {
                    emit signalTipsChanged("选择图像失败！");
                    return false;
                }
                emit signalTipsChanged("亮场采集结束");

                // Step 6: Stop X-ray and acquire dark field
                qDebug() << "[Defect] Group" << (groupIdx + 1) << "- Stopping X-ray source";
                emit signalTipsChanged("等待射线源关闭");

                IXS120BP120P366::Instance().stopXRay();
                QThread::msleep(2000);
                emit signalTipsChanged("射线源已关闭");

                qDebug() << "[Defect] Group" << (groupIdx + 1) << "- Acquiring dark field";
                emit signalTipsChanged("正在采集暗场");

                if (!DET.DefectForceDarkContinuousAcq(groupIdx))
                {
                    emit signalTipsChanged("暗场采集失败！");
                    return false;
                }
                emit signalTipsChanged("暗场采集结束");
            }

            // Step 7: Generate defect correction
            if (DET.CurrentTransaction() != 2)
            {
                return false;
            }

            qDebug() << "[Defect] Generating defect correction";
            emit signalTipsChanged("正在生成缺陷校正");

            bool result = DET.DefectGeneration();
            qDebug() << "[Defect] Calibration completed. Result:" << result;

            return result;
        });

    // Setup watcher for completion
    auto* watcher = new QFutureWatcher<bool>(this);
    connect(watcher, &QFutureWatcher<bool>::finished, this,
            [this, watcher]()
            {
                ui.pushButton_Defect->setEnabled(true);
                ui.pushButton_DefectAbort->setEnabled(false);

                stopXRaySource();

                bool result = watcher->future().result();
                qDebug() << "[Defect] Workflow finished. Result:" << result;

                if (result)
                {
                    emit signalTipsChanged("缺陷校正流程完成");
                }
                else
                {
                    emit signalTipsChanged("缺陷校正流程失败！");
                }

                disconnect(&DET, &NDT1717MA::signalAcqImageReceived, this,
                           &CreateCorrectTemplateDlg::onDefectAcqImageReceived);
                watcher->deleteLater();
            });
    watcher->setFuture(future);
}

// ============================================================================
// UI Update Methods
// ============================================================================

void CreateCorrectTemplateDlg::ShowTips(const QString& msg)
{
    ui.label_Tips->setText(msg);
}

void CreateCorrectTemplateDlg::ModifyGainVoltageCurrent(int voltage, int current)
{
    ui.spinBox_GainVoltage->setValue(voltage);
    ui.spinBox_GainCurrent->setValue(current);
}

void CreateCorrectTemplateDlg::ModifyDefectVoltageCurrent(int voltage, int current)
{
    ui.spinBox_DefectVoltage->setValue(voltage);
    ui.spinBox_DefectCurrent->setValue(current);
}

void CreateCorrectTemplateDlg::ModifyMode(int modeIdx)
{
    // Convert combo box index to detector mode (Mode5-Mode8)
    int modeNumber = modeIdx + 5;
    std::string mode = "Mode" + std::to_string(modeNumber);

    qDebug() << "[Mode] Changing detector mode to" << QString::fromStdString(mode) << "(index:" << modeIdx << ")";

    // Execute mode change in background thread
    auto future = QtConcurrent::run(
        [this, mode]()
        {
            if (!DET.Initialized())
            {
                qDebug() << "[Mode] Error: Detector not initialized";
                emit signalTipsChanged("探测器未连接！");
                return false;
            }

            if (!DET.CanModifyCfg())
            {
                qDebug() << "[Mode] Error: Cannot modify configuration in current state";
                emit signalTipsChanged("当前不允许修改参数！");
                return false;
            }

            qDebug() << "[Mode] Updating detector mode to" << QString::fromStdString(mode);
            return DET.UpdateMode(mode);
        });

    auto* watcher = new QFutureWatcher<bool>(this);
    connect(watcher, &QFutureWatcher<bool>::finished, this,
            [this, watcher, mode]()
            {
                bool result = watcher->future().result();
                qDebug() << "[Mode] Mode change completed. Result:" << result;

                if (result)
                {
                    emit signalTipsChanged("修改探测器模式成功！");
                }
                else
                {
                    emit signalTipsChanged("修改探测器模式失败！");
                }

                watcher->deleteLater();
            });
    watcher->setFuture(future);

    future.waitForFinished();
}

// ============================================================================
// Event Handlers - Image Selection Progress
// ============================================================================

void CreateCorrectTemplateDlg::onOffsetImageSelected(int total, int valid)
{
    ui.lineEdit_OffsetProgress->setText(QString("%1 / %2").arg(valid).arg(total));
}

void CreateCorrectTemplateDlg::onGainImageSelected(int total, int valid)
{
    ui.lineEdit_GainProgress->setText(QString("%1 / %2").arg(valid).arg(total));
}

void CreateCorrectTemplateDlg::onDefectImageSelected(int total, int valid)
{
    ui.lineEdit_DefectProgress->setText(QString("%1 / %2").arg(valid).arg(total));
}

// ============================================================================
// Event Handlers - Image Acquisition
// ============================================================================

void CreateCorrectTemplateDlg::onGainAcqImageReceived(QImage image, int idx, int grayValue)
{
    // Update gray value display immediately (lightweight operation)
    ui.lineEdit_GainCenterValue->setText(QString::number(grayValue));
    nCurrentGray = grayValue;

    // Process image in background thread to avoid UI blocking
    const int viewWidth = ui.graphicsView_GainImageView->width() - 5;
    const int viewHeight = ui.graphicsView_GainImageView->height() - 5;

    auto future = QtConcurrent::run(
        [image, viewWidth, viewHeight]() -> QPixmap
        {
            // Calculate window/level from image statistics for optimal display
            int maxVal = -1;
            int minVal = -1;
            XImageHelper::calculateMaxMinValue(image, maxVal, minVal);

            int width = 0;
            int level = 0;
            XImageHelper::calculateWLAdvanced(maxVal, minVal, width, level,
                                              1);  // mode 1: optimized display (85% range)

            // Apply window/level adjustment and convert to 8-bit for display
            QImage displayImage = XImageHelper::adjustWL(image, width, level);
            if (displayImage.isNull())
            {
                return QPixmap();
            }

            return QPixmap::fromImage(displayImage)
                .scaled(viewWidth, viewHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        });

    auto* watcher = new QFutureWatcher<QPixmap>(this);
    connect(watcher, &QFutureWatcher<QPixmap>::finished, this,
            [this, watcher]()
            {
                QPixmap pixmap = watcher->future().result();
                if (!pixmap.isNull())
                {
                    gainPixmapItem->setPixmap(pixmap);
                }
                watcher->deleteLater();
            });
    watcher->setFuture(future);
}

void CreateCorrectTemplateDlg::onDefectAcqImageReceived(QImage image, int idx, int grayValue)
{
    // Update gray value display immediately (lightweight operation)
    ui.lineEdit_DefectCurrentGray->setText(QString::number(grayValue));
    nCurrentGray = grayValue;

    // Process image in background thread to avoid UI blocking
    const int viewWidth = ui.graphicsView_DefectImageView->width() - 5;
    const int viewHeight = ui.graphicsView_DefectImageView->height() - 5;

    auto future = QtConcurrent::run(
        [image, viewWidth, viewHeight]() -> QPixmap
        {
            // Calculate window/level from image statistics for optimal display
            int maxVal = -1;
            int minVal = -1;
            XImageHelper::calculateMaxMinValue(image, maxVal, minVal);

            int width = 0;
            int level = 0;
            XImageHelper::calculateWLAdvanced(maxVal, minVal, width, level,
                                              1);  // mode 1: optimized display (85% range)

            // Apply window/level adjustment and convert to 8-bit for display
            QImage displayImage = XImageHelper::adjustWL(image, width, level);
            if (displayImage.isNull())
            {
                return QPixmap();
            }

            return QPixmap::fromImage(displayImage)
                .scaled(viewWidth, viewHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        });

    auto* watcher = new QFutureWatcher<QPixmap>(this);
    connect(watcher, &QFutureWatcher<QPixmap>::finished, this,
            [this, watcher]()
            {
                QPixmap pixmap = watcher->future().result();
                if (!pixmap.isNull())
                {
                    defectPixmapItem->setPixmap(pixmap);
                }
                watcher->deleteLater();
            });
    watcher->setFuture(future);
}

void CreateCorrectTemplateDlg::onDefectGroupChanged(int groupIdx, int totalGroups)
{
    ui.lineEdit_DefectExceptedGray->setText(QString::number(DET.nDefectExpectedGrays[groupIdx]));
    ui.lineEdit_DefectGroup->setText(QString("%1 / %2").arg(groupIdx + 1).arg(totalGroups));
}
