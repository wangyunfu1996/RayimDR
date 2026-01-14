#pragma once

#include <QObject>

#define DET DetManager::Instance()

struct DetStatus
{
	QString vendor;
	QString model;
	bool connected{ false };
	QString ip;
	int port{ 0 };
};

class DetManager : public QObject
{
	Q_OBJECT

private:
	DetManager(QObject* parent = nullptr);
	~DetManager();

public:
	static DetManager& Instance();
	const DetStatus& Status() const;

private:
	void connectToDet();
	void disconnectFromDet();

signals:
	void signalConnectToDet();
	void signalDetConnected();
	void signalDisconnectFromDet();

private:
	DetStatus detStatus;
};

