#include "AcqTask.h"

#include <QtConcurrent/QtConcurrent>
#include <qdebug.h>
#include <qthread.h>
#include <qfuture.h>

#include "AcqTaskManager.h"
#include "ImageRender/XImageHelper.h"

#include "../IRayDetector/IRayDetector.h"

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

	// 单次采集
    if (acqCondition.frame == 1)
    {
#if DET_TYPE == DET_TYPE_VIRTUAL
		int receivedIdx{ 0 };
		QImage receivedImage = XImageHelper::generateRandomGaussianGrayImage(DET_WIDTH, DET_HEIGHT);
		qDebug() << "单次采集结束";
		AcqTaskManager::Instance().receivedImageList[receivedIdx] = receivedImage;
		emit AcqTaskManager::Instance().signalAcqTaskReceivedIdxChanged(acqCondition, receivedIdx);
#elif DET_TYPE == DET_TYPE_IRAY
		connect(&DET, &IRayDetector::signalAcqImageReceived, this, [this, acqCondition](int receivedIdx) {
			QImage receivedImage = DET.getReceivedImage();
			AcqTaskManager::Instance().receivedImageList[receivedIdx] = receivedImage;
			emit AcqTaskManager::Instance().signalAcqTaskReceivedIdxChanged(acqCondition, receivedIdx);
			});
		DET.ClearAcq();
		emit AcqTaskManager::Instance().signalAcqTaskStopped();
#endif // DET_TYPE
	}
	// 连续采集
	else if (acqCondition.frame == -1)
	{
#if DET_TYPE == DET_TYPE_VIRTUAL
		int receivedIdx{ 0 };
		while (!stopRequested.load())
		{
			int vecIdx = (receivedIdx + 1) % IMAGE_BUFFER_SIZE;
			qDebug() << "第 " << receivedIdx + 1 << " 帧数据采集完成，在数组中的存储索引为：" << vecIdx;
			QImage randomImage = XImageHelper::generateRandomGaussianGrayImage(DET_WIDTH, DET_HEIGHT);
			AcqTaskManager::Instance().receivedImageList[vecIdx] = randomImage;
			emit AcqTaskManager::Instance().signalAcqTaskReceivedIdxChanged(acqCondition, receivedIdx);
			receivedIdx++;
		}
		emit AcqTaskManager::Instance().signalAcqTaskStopped();
#elif DET_TYPE == DET_TYPE_IRAY
		connect(&DET, &IRayDetector::signalAcqImageReceived, this, [this, acqCondition](int receivedIdx) {
			QImage receivedImage = DET.getReceivedImage();
			AcqTaskManager::Instance().receivedImageList[receivedIdx] = receivedImage;
			emit AcqTaskManager::Instance().signalAcqTaskReceivedIdxChanged(acqCondition, receivedIdx);
			});
		DET.StartAcq();
		// 立即返回让线程事件循环运行，保证 queued 信号能实时派发
		return;
#endif // DET_TYPE
	}
	// 指定张数的采集
	else
	{
		for (int frameIdx(0); frameIdx < acqCondition.frame; frameIdx++)
		{
			if (stopRequested.load())
			{
				qDebug() << "采集即将停止";
				break;
			}
			int vecIdx = (frameIdx + 1) % IMAGE_BUFFER_SIZE;
			qDebug() << "第 " << frameIdx + 1 << " 个角度的数据采集完成，共 " << acqCondition.frame << " 个角度，在数组中的存储索引为：" << vecIdx;
			QImage randomImage = XImageHelper::generateRandomGaussianGrayImage(DET_WIDTH, DET_HEIGHT);
			AcqTaskManager::Instance().receivedImageList[vecIdx] = randomImage;
			emit AcqTaskManager::Instance().signalAcqTaskReceivedIdxChanged(acqCondition, frameIdx);

			if (acqCondition.saveToFiles)
			{
				QString fileName = QString("%1/Image_%2_%3x%4.raw")
					.arg(acqCondition.savePath)
					.arg(frameIdx)
					.arg(DET_WIDTH)
					.arg(DET_HEIGHT);
				// 开启后台任务 将图像保存到文件
				QtConcurrent::run(XImageHelper::Instance().saveImageU16Raw, randomImage, fileName);
			}
		}
		emit AcqTaskManager::Instance().signalAcqTaskStopped();
	}

}

void AcqTask::stopAcq()
{
	stopRequested.store(true);

#if DET_TYPE == DET_TYPE_IRAY
	DET.StopAcq();
#endif

	emit AcqTaskManager::Instance().signalAcqTaskStopped();
}

