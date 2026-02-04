#pragma once

#include <QObject>

#include "XGlobal.h"

class QThread;
class AcqTask;

class AcqTaskManager : public QObject
{
    Q_OBJECT

private:
    AcqTaskManager(QObject* parent = nullptr);
    ~AcqTaskManager();

public:
    static AcqTaskManager& Instance();

    void startAcq();
    void stopAcq();

    bool isAcquiring() const;

    void updateAcqCond(const AcqCondition& acqCond);
    void updateAcqType(AcqType acqType);
    void updateAcqVoltage(int voltage);
    void updateAcqCurrent(int current);
    void updateAcqFrameRate(int frameRate);
    void updateAcqFrame(int frame);
    void updateAcqStackedFrame(int stackedFrame);
    void updateAcqDetMode(std::string mode);
    QImage receivedImage(int idx);

signals:
    void signalAcqTaskReceivedIdxChanged(AcqCondition condition, int receivedIdx);
    void signalAcqTaskStopped();
    void signalAcqErr(const QString& msg);
    void signalAcqProgressChanged(const QString& msg);

private:
    std::atomic_bool acquiring{false};
    AcqCondition* acqCondition{nullptr};
    AcqTask* acqTask{nullptr};

    QVector<QImage> receivedImageList;
    QVector<QImage> stackedImageList;

    friend class AcqTask;
};
