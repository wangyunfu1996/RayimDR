#include "XRayManager.h"
#include "XRayWorker.h"

#include <qdebug.h>
#include <qthread.h>

XRayManager::XRayManager(QObject* parent)
	: QObject(parent)
{
	xrayThread_.setObjectName("XRayThread");
	worker_ = new XRayWorker(this);
	worker_->moveToThread(&xrayThread_);
	connect(&xrayThread_, &QThread::finished, worker_, &QObject::deleteLater);

	connect(this, &XRayManager::signalConnectToXRaySource, worker_, &XRayWorker::connectToXRaySource, Qt::QueuedConnection);
	connect(this, &XRayManager::signalDisconnectFromXRaySource, worker_, &XRayWorker::disconnectFromXRaySource, Qt::QueuedConnection);
	connect(this, &XRayManager::signalXRaySourceWarmup, worker_, &XRayWorker::warmup, Qt::QueuedConnection);
	connect(this, &XRayManager::signalXRaySourceOpen, worker_, &XRayWorker::open, Qt::QueuedConnection);
	connect(this, &XRayManager::signalXRaySourceClose, worker_, &XRayWorker::close, Qt::QueuedConnection);
	connect(this, &XRayManager::signalXRaySourceUpdateTargetTubeVoltage, worker_, &XRayWorker::updateTargetTubeVoltage, Qt::QueuedConnection);
	connect(this, &XRayManager::signalXRaySourceUpdateTargetTubeCurrent, worker_, &XRayWorker::updateTargetTubeCurrent, Qt::QueuedConnection);

	xrayThread_.start();
}

XRayManager::~XRayManager()
{
	xrayThread_.quit();
	xrayThread_.wait();
}

XRayManager& XRayManager::Instance()
{
	static XRayManager instance;
	return instance;
}

const XRayStatus& XRayManager::Status() const
{
	return status;
}

void XRayManager::onWorkerStatusChanged(const XRayStatus& newStatus)
{
	std::unique_lock<std::shared_mutex> lock(_rw_mutex);
	status = newStatus;
	lock.unlock();
	emit signalStatusChanged(status);
}

