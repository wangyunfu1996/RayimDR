#pragma once

#include <shared_mutex>
#include <QObject>
#include <QThread>

struct XRayStatus
{
	bool connected{ false };
	float targetTubeVoltage{ 0.0 };		// kV
	float currentTubeVoltage{ 0.0 };	// kV

	float targetTubeCurrent{ 0.0 };		// mA
	float currentTubeCurrent{ 0.0 };	// mA

	float targetTubePower{ 0.0 };		// kW
	float currentTubePower{ 0.0 };		// kW
};

#define xRay XRayManager::Instance()
#define pXRay &XRayManager::Instance()

class XRayManager : public QObject
{
	Q_OBJECT

private:
	XRayManager(QObject* parent = nullptr);
	~XRayManager();

public:
	static XRayManager& Instance();
	const XRayStatus& Status() const;

signals:
	void signalConnectToXRaySource();
	void signalDisconnectFromXRaySource();
	void signalXRaySourceWarmup();
	void signalXRaySourceOpen();
	void signalXRaySourceClose();
	void signalXRaySourceUpdateTargetTubeVoltage(float tubeVoltage);
	void signalXRaySourceUpdateTargetTubeCurrent(float tubeCurrent);
	void signalStatusChanged(const XRayStatus& status);

public slots:
	void onWorkerStatusChanged(const XRayStatus& status);

private:
	XRayStatus status;
	mutable std::shared_mutex _rw_mutex;
	QThread xrayThread_;
	class XRayWorker* worker_{ nullptr };
};

