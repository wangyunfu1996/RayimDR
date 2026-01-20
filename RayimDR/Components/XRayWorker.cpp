#include "XRayWorker.h"
#include "XRayManager.h"

#include <QSettings>
#include <QThread>
#include <QMetaObject>
#include <qdebug.h>

XRayWorker::XRayWorker(XRayManager* owner)
	: QObject(nullptr)
	, owner_(owner)
{
}

void XRayWorker::publishStatus(const XRayStatus& status)
{
	if (!owner_)
	{
		return;
	}

	QMetaObject::invokeMethod(owner_, [owner = owner_, status]() {
		owner->onWorkerStatusChanged(status);
	}, Qt::QueuedConnection);
}

void XRayWorker::connectToXRaySource()
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

	XRayStatus newStatus;
	newStatus.connected = true;

	// TODO: 调用真实 SDK 完成连接
	QThread::msleep(500);

	qDebug() << "射线源连接状态：" << newStatus.connected;
	publishStatus(newStatus);
}

void XRayWorker::disconnectFromXRaySource()
{
	qDebug("开始断开射线源的连接");
	XRayStatus newStatus;
	newStatus.connected = false;

	// TODO: 调用真实 SDK 完成断连

	publishStatus(newStatus);
}

void XRayWorker::warmup()
{
	qDebug() << "执行射线源预热";
	// TODO: 实现预热逻辑
}

void XRayWorker::open()
{
	qDebug() << "打开射线源";
	// TODO: 实现打开逻辑
}

void XRayWorker::close()
{
	qDebug() << "关闭射线源";
	// TODO: 实现关闭逻辑
}

void XRayWorker::updateTargetTubeVoltage(float tubeVoltage)
{
	qDebug() << "设置靶电压" << tubeVoltage;
	// TODO: SDK 设置靶电压
}

void XRayWorker::updateTargetTubeCurrent(float tubeCurrent)
{
	qDebug() << "设置靶电流" << tubeCurrent;
	// TODO: SDK 设置靶电流
}
