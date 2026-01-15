#include "AcqTask.h"

#include <QtConcurrent/QtConcurrent>
#include <qdebug.h>
#include <qthread.h>
#include <qfuture.h>

#include "AcqTaskManager.h"
#include "ImageRender/XImageHelper.h"

#include "../IRayDetector/IRayDetector.h"
#include "../IRayDetector/TiffHelper.h"

AcqTask::AcqTask(QObject* parent)
	: QObject(parent)
{
	qDebug() << "构造采集任务";
}

AcqTask::~AcqTask()
{
	disconnect(&DET, nullptr, nullptr, nullptr);
	qDebug() << "采集任务完成，清理资源";
}

void AcqTask::doAcq(AcqCondition acqCondition)
{
	qDebug() << "开启采集任务，当前线程：" << QThread::currentThreadId();
	qDebug() << acqCondition;

#if DET_TYPE == DET_TYPE_VIRTUAL
	int receivedIdx{ 0 };
	while (!stopRequested.load())
	{
		receivedIdx++;
		int vecIdx = receivedIdx % IMAGE_BUFFER_SIZE;
		qDebug() << "第 " << receivedIdx << " 帧数据采集完成，在数组中的存储索引为：" << vecIdx;
		QImage randomImage = XImageHelper::generateRandomGaussianGrayImage(DET_WIDTH, DET_HEIGHT);
		AcqTaskManager::Instance().receivedImageList[vecIdx] = randomImage;
		emit AcqTaskManager::Instance().signalAcqTaskReceivedIdxChanged(acqCondition, receivedIdx);

		// 保存文件（仅在指定张数采集时）
		if (acqCondition.saveToFiles && acqCondition.frame != INT_MAX)
		{
			if (acqCondition.saveType == ".RAW")
			{
				QString fileName = QString("%1/Image%2_%3x%4.raw")
					.arg(acqCondition.savePath)
					.arg(receivedIdx)
					.arg(DET_WIDTH)
					.arg(DET_HEIGHT);
				QtConcurrent::run(XImageHelper::Instance().saveImageU16Raw, randomImage, fileName);
			}
			else if (acqCondition.saveType == ".TIFF")
			{
				QString fileName = QString("%1/Image%2.tif")
					.arg(acqCondition.savePath)
					.arg(receivedIdx);
				QtConcurrent::run(TiffHelper::SaveImage, randomImage, fileName.toStdString());
			}
		}

		if (receivedIdx == acqCondition.frame)
		{
			stopAcq();
		}
	}
#elif DET_TYPE == DET_TYPE_IRAY
	QPointer<AcqTask> weakThis(this);
	connect(&DET, &IRayDetector::signalAcqImageReceived, this, [weakThis, acqCondition](int receivedIdx) {
		if (!weakThis || weakThis->stopRequested.load())
		{
			qDebug() << "收到探测器数据，但是采集任务已经被停止";
			return;
		}
		QImage receivedImage = DET.getReceivedImage();
		AcqTaskManager::Instance().receivedImageList[receivedIdx] = receivedImage;
		emit AcqTaskManager::Instance().signalAcqTaskReceivedIdxChanged(acqCondition, receivedIdx);

		// 保存文件（仅在指定张数采集时）
		if (acqCondition.saveToFiles && acqCondition.frame != INT_MAX)
		{
			QString fileName = QString("%1/Image%2_%3x%4.raw")
				.arg(acqCondition.savePath)
				.arg(receivedIdx)
				.arg(DET_WIDTH)
				.arg(DET_HEIGHT);
			QtConcurrent::run(XImageHelper::Instance().saveImageU16Raw, receivedImage, fileName);
		}

		if (receivedIdx == acqCondition.frame)
		{
			qDebug() << "已采集足够数据，停止采集";
			weakThis->stopAcq();
		}
		});
	DET.StartAcq();
	return;
#endif // DET_TYPE
}

void AcqTask::stopAcq()
{
	stopRequested.store(true);

#if DET_TYPE == DET_TYPE_IRAY
	DET.StopAcq();
#endif

	emit AcqTaskManager::Instance().signalAcqTaskStopped();
}

