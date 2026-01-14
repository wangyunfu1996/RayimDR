#include "DetManager.h"

#include <qsettings.h>
#include <qdebug.h>

DetManager::DetManager(QObject* parent)
	: QObject(parent)
{
	connect(this, &DetManager::signalConnectToDet, this, &DetManager::connectToDet);
	connect(this, &DetManager::signalDisconnectFromDet, this, &DetManager::disconnectFromDet);
}

DetManager::~DetManager()
{}

DetManager& DetManager::Instance()
{
	static DetManager instance;
	return instance;
}

const DetStatus& DetManager::Status() const
{
	return detStatus;
}

void DetManager::connectToDet()
{
	QSettings settings("xconfig.ini", QSettings::IniFormat);
	QString vendor = settings.value("det/vendor").toString();
	QString model = settings.value("det/model").toString();
	QString ip = settings.value("det/ip").toString();
	int port = settings.value("det/port").toInt();
	qDebug() << "开始连接探测器，"
		<< "vendor: " << vendor
		<< "model: " << model
		<< "ip: " << ip
		<< "port: " << port;

	detStatus.vendor = vendor;
	detStatus.model = model;
	detStatus.ip = ip;
	detStatus.port = port;

	if (vendor == "iRay")
	{
		detStatus.connected = true;
	}
}

void DetManager::disconnectFromDet()
{
	detStatus.connected = false;
}

