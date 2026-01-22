#include "AcqTaskManager.h"

#include <qthread.h>
#include <qimage.h>
#include <qdebug.h>

#include "XRayManager.h"
#include "XSignalsHelper.h"
#include "XGlobal.h"

#include "../IRayDetector/NDT1717MA.h"
#include "AcqTask.h"

AcqTaskManager::AcqTaskManager(QObject* parent)
	: QObject(parent), acqCondition(new AcqCondition())
{
	// 关键：注册 AcqCondition 为元类型
	qRegisterMetaType<AcqCondition>("AcqCondition");

	receivedImageList.reserve(IMAGE_BUFFER_SIZE);
	receivedImageList.resize(IMAGE_BUFFER_SIZE);
}

AcqTaskManager::~AcqTaskManager()
{
	delete acqCondition;
	acqCondition = nullptr;
}

AcqTaskManager& AcqTaskManager::Instance()
{
	static AcqTaskManager instance;
	return instance;
}

void AcqTaskManager::startAcq()
{
	if (acquiring.load())
	{
		emit xSignaHelper.signalShowErrorMessageBar("当前正在进行采集，请等待当前采集结束!");
		return;
	}

	acquiring.store(true);
	acqThread = new QThread();
	acqTask = new AcqTask();
	acqTask->moveToThread(acqThread);

	connect(acqThread, &QThread::started, acqTask, [this]() {
		acqTask->startAcq(*acqCondition);
		});

	// 连接采集停止信号：先让线程退出
	connect(this, &AcqTaskManager::signalAcqTaskStopped, acqThread, &QThread::quit);

	// 线程结束后清理资源：先删除 task 再删除 thread
	connect(acqThread, &QThread::finished, acqTask, &QObject::deleteLater);
	connect(acqThread, &QThread::finished, acqThread, &QObject::deleteLater);
	connect(acqThread, &QThread::finished, this, [this]() {
		acquiring.store(false);
		acqTask = nullptr;
		acqThread = nullptr;
		});

	acqThread->start();
}

void AcqTaskManager::stopAcq()
{
	if (!acquiring.load())
	{
		emit xSignaHelper.signalShowErrorMessageBar("当前没有正在进行的采集!");
		return;
	}

	acqTask->stopAcq();
}

bool AcqTaskManager::isAcquiring() const
{
	return acquiring.load();
}

void AcqTaskManager::updateAcqCond(const AcqCondition& acqCond)
{
	*acqCondition = acqCond;
}

void AcqTaskManager::updateAcqType(AcqType acqType)
{
	acqCondition->acqType = acqType;
}

void AcqTaskManager::updateAcqVoltage(int voltage)
{
	acqCondition->voltage = voltage;
}

void AcqTaskManager::updateAcqCurrent(int current)
{
	acqCondition->current = current;
}

void AcqTaskManager::updateAcqFrameRate(int frameRate)
{
	acqCondition->frameRate = frameRate;
}

void AcqTaskManager::updateAcqFrame(int frame)
{
	acqCondition->frame = frame;
}

void AcqTaskManager::updateAcqStackedFrame(int stackedFrame)
{
	acqCondition->stackedFrame = stackedFrame;
}

void AcqTaskManager::updateAcqDetMode(std::string mode)
{
	acqCondition->mode = mode;
}

QImage AcqTaskManager::receivedImage(int idx)
{
	if (idx < 0)
	{
		qDebug() << "尝试获取的图像索引错误：" << idx << receivedImageList.size();
		return QImage();
	}
	int vecIdx = idx % IMAGE_BUFFER_SIZE;
	qDebug() << "读取图像：" << idx << "，在vec中的索引：" << vecIdx;
	return receivedImageList.at(vecIdx);
}

