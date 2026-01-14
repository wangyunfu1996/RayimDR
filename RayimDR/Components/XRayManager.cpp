#include "XRayManager.h"

#include <qdebug.h>
#include <qthread.h>
#include <qsettings.h>

XRayManager::XRayManager(QObject* parent)
	: QObject(parent)
{
	connect(this, &XRayManager::signalConnectToXRaySource, this, &XRayManager::ConnectToXRaySource);
	connect(this, &XRayManager::signalDisconnectFromXRaySource, this, &XRayManager::DisconnectFromXRaySource);
}

XRayManager::~XRayManager()
{}

XRayManager& XRayManager::Instance()
{
	static XRayManager instance;
	return instance;
}

const XRayStatus& XRayManager::Status() const
{
	return status;
}

void XRayManager::ConnectToXRaySource()
{
	QSettings settings("xconfig.ini", QSettings::IniFormat);
	QString vendor = settings.value("det/vendor").toString();
	QString model = settings.value("det/model").toString();
	QString ip = settings.value("det/ip").toString();
	int port = settings.value("det/port").toInt();

	qDebug() << "开始连接射线源，"
		<< "vendor: " << vendor
		<< "model: " << model
		<< "ip: " << ip
		<< "port: " << port;

	std::unique_lock<std::shared_mutex> lock(_rw_mutex);

	// TODO
	QThread::currentThread()->msleep(500);

	status.connected = true;
	qDebug() << "射线源连接状态：" << status.connected;
}

void XRayManager::DisconnectFromXRaySource()
{
	qDebug("开始断开射线源的连接");
	std::unique_lock<std::shared_mutex> lock(_rw_mutex);

	// TODO

	status.connected = false;
}

void XRayManager::XRaySourceWarmup()
{
}

void XRayManager::XRaySourceOpen()
{
}

void XRayManager::XRaySourceClose()
{
}

void XRayManager::XRaySourceUpdateTargetTubeVoltage()
{
}

void XRayManager::XRaySourceUpdateTargetTubeCurrent()
{
}

