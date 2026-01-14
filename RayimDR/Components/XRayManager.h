#pragma once

#include <shared_mutex>
#include <QObject>

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

private:
	void ConnectToXRaySource();
	void DisconnectFromXRaySource();
	void XRaySourceWarmup();
	void XRaySourceOpen();
	void XRaySourceClose();
	void XRaySourceUpdateTargetTubeVoltage();
	void XRaySourceUpdateTargetTubeCurrent();

signals:
	void signalConnectToXRaySource();
	void signalDisconnectFromXRaySource();
	void signalXRaySourceWarmup();
	void signalXRaySourceOpen();
	void signalXRaySourceClose();
	void signalXRaySourceUpdateTargetTubeVoltage(float tubeVoltage);
	void signalXRaySourceUpdateTargetTubeCurrent(float tubeCurrent);

private:
	XRayStatus status;
	mutable std::shared_mutex _rw_mutex;
};

