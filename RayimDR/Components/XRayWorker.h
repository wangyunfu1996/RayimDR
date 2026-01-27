#pragma once

#include <QObject>

struct XRayStatus;
class XRayManager;

class XRayWorker : public QObject
{
    Q_OBJECT

public:
    explicit XRayWorker(XRayManager* owner);

    void connectToXRaySource();
    void disconnectFromXRaySource();
    void warmup();
    void open();
    void close();
    void updateTargetTubeVoltage(float tubeVoltage);
    void updateTargetTubeCurrent(float tubeCurrent);

private:
    void publishStatus(const XRayStatus& status);

private:
    XRayManager* owner_{nullptr};
};
