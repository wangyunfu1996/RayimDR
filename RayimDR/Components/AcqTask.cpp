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
	AcqTaskManager::Instance().stackedImageList.clear();
	qDebug() << "采集任务完成，清理资源";
}

void AcqTask::doAcq(AcqCondition acqCondition)
{
	qDebug() << "开启采集任务，当前线程：" << QThread::currentThreadId();
	qDebug() << acqCondition;

#if DET_TYPE == DET_TYPE_VIRTUAL
	int localReceivedIdx{ 0 };
	int rawFrameIdx{ 0 };  // 原始帧索引
	AcqTaskManager::Instance().stackedImageList.clear();
	
	// 计算需要叠加的总帧数：stackedFrame=0时采集1帧，stackedFrame=n时采集1+n帧
	int totalStackFrames = (acqCondition.stackedFrame == 0) ? 1 : (1 + acqCondition.stackedFrame);
	qDebug() << "叠加配置：stackedFrame=" << acqCondition.stackedFrame << "，每次需要采集" << totalStackFrames << "帧进行叠加";
	
	while (!stopRequested.load())
	{
		rawFrameIdx++;
		QImage rawImage = XImageHelper::generateRandomGaussianGrayImage(DET_WIDTH, DET_HEIGHT);
		AcqTaskManager::Instance().stackedImageList.append(rawImage);
		qDebug() << "接收原始帧 " << rawFrameIdx << "，叠加缓冲区当前帧数：" << AcqTaskManager::Instance().stackedImageList.size();
		
		// 当收集满需要的帧数后，进行叠加处理
		if (AcqTaskManager::Instance().stackedImageList.size() == totalStackFrames)
		{
			localReceivedIdx++;
			int vecIdx = localReceivedIdx % IMAGE_BUFFER_SIZE;
			qDebug() << "叠加完成第 " << localReceivedIdx << " 帧数据（由" << totalStackFrames << "帧叠加而成），在数组中的存储索引为：" << vecIdx;
			
			// 执行叠加：累加所有帧的像素值后求平均
			QImage stackedImage = stackImages(AcqTaskManager::Instance().stackedImageList);
			AcqTaskManager::Instance().receivedImageList[vecIdx] = stackedImage;
			AcqTaskManager::Instance().stackedImageList.clear();
			
			emit AcqTaskManager::Instance().signalAcqTaskReceivedIdxChanged(acqCondition, localReceivedIdx);

			// 保存文件（仅在指定张数采集时）
			if (acqCondition.saveToFiles && acqCondition.frame != INT_MAX)
			{
				if (acqCondition.saveType == ".RAW")
				{
					QString fileName = QString("%1/Image%2_%3x%4.raw")
						.arg(acqCondition.savePath)
						.arg(localReceivedIdx)
						.arg(DET_WIDTH)
						.arg(DET_HEIGHT);
					QtConcurrent::run(XImageHelper::Instance().saveImageU16Raw, stackedImage, fileName);
				}
				else if (acqCondition.saveType == ".TIFF")
				{
					QString fileName = QString("%1/Image%2.tif")
						.arg(acqCondition.savePath)
						.arg(localReceivedIdx);
					QtConcurrent::run(TiffHelper::SaveImage, stackedImage, fileName.toStdString());
				}
			}

			if (localReceivedIdx == acqCondition.frame)
			{
				stopAcq();
			}
		}
		
		QThread::msleep(10); // 避免CPU占用过高
	}
#elif DET_TYPE == DET_TYPE_IRAY
	QPointer<AcqTask> weakThis(this);
	AcqTaskManager::Instance().stackedImageList.clear();
	rawFrameCount.store(0);
	receivedIdx.store(0);
	
	// 计算需要叠加的总帧数：stackedFrame=0时采集1帧，stackedFrame=n时采集1+n帧
	int totalStackFrames = (acqCondition.stackedFrame == 0) ? 1 : (1 + acqCondition.stackedFrame);
	qDebug() << "叠加配置：stackedFrame=" << acqCondition.stackedFrame << "，每次需要采集" << totalStackFrames << "帧进行叠加";
	
	connect(&DET, &IRayDetector::signalAcqImageReceived, this, [weakThis, acqCondition, totalStackFrames](int detectorFrameIdx) {
		if (!weakThis || weakThis->stopRequested.load())
		{
			qDebug() << "收到探测器数据，但是采集任务已经被停止";
			return;
		}
		
		QImage rawImage = DET.getReceivedImage();
		AcqTaskManager::Instance().stackedImageList.append(rawImage);
		int currentRawCount = weakThis->rawFrameCount.fetch_add(1) + 1;
		qDebug() << "接收原始帧 " << currentRawCount << "，叠加缓冲区当前帧数：" << AcqTaskManager::Instance().stackedImageList.size();
		
		// 当收集满需要的帧数后，进行叠加处理
		if (AcqTaskManager::Instance().stackedImageList.size() == totalStackFrames)
		{
			int currentReceivedIdx = weakThis->receivedIdx.fetch_add(1) + 1;
			if (currentReceivedIdx == acqCondition.frame)
			{
				qDebug() << "已采集足够数据，停止采集";
				weakThis->stopAcq();
			}

			int vecIdx = currentReceivedIdx % IMAGE_BUFFER_SIZE;
			qDebug() << "叠加完成第 " << currentReceivedIdx << " 帧数据（由" << totalStackFrames << "帧叠加而成），在数组中的存储索引为：" << vecIdx;
			
			// 执行叠加：累加所有帧的像素值后求平均
			QImage stackedImage = weakThis->stackImages(AcqTaskManager::Instance().stackedImageList);
			AcqTaskManager::Instance().receivedImageList[vecIdx] = stackedImage;
			AcqTaskManager::Instance().stackedImageList.clear();
			
			emit AcqTaskManager::Instance().signalAcqTaskReceivedIdxChanged(acqCondition, currentReceivedIdx);

			// 保存文件（仅在指定张数采集时）
			if (acqCondition.saveToFiles && acqCondition.frame != INT_MAX)
			{
				if (acqCondition.saveType == ".RAW")
				{
					QString fileName = QString("%1/Image%2_%3x%4.raw")
						.arg(acqCondition.savePath)
						.arg(currentReceivedIdx)
						.arg(DET_WIDTH)
						.arg(DET_HEIGHT);
					QtConcurrent::run(XImageHelper::Instance().saveImageU16Raw, stackedImage, fileName);
				}
				else if (acqCondition.saveType == ".TIFF")
				{
					QString fileName = QString("%1/Image%2.tif")
						.arg(acqCondition.savePath)
						.arg(currentReceivedIdx);
					QtConcurrent::run(TiffHelper::SaveImage, stackedImage, fileName.toStdString());
				}
			}

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

QImage AcqTask::stackImages(const QVector<QImage>& images)
{
	if (images.isEmpty())
	{
		qWarning() << "叠加图像列表为空";
		return QImage();
	}
	
	if (images.size() == 1)
	{
		return images[0];
	}
	
	int width = images[0].width();
	int height = images[0].height();
	QImage::Format format = images[0].format();
	
	// 创建累加缓冲区（使用32位整数避免溢出）
	QVector<quint32> accumulateBuffer(width * height, 0);
	
	// 累加所有图像
	for (const QImage& img : images)
	{
		if (img.width() != width || img.height() != height || img.format() != format)
		{
			qWarning() << "叠加图像尺寸或格式不一致，跳过该帧";
			continue;
		}
		
		const quint16* srcData = reinterpret_cast<const quint16*>(img.constBits());
		for (int i = 0; i < width * height; ++i)
		{
			accumulateBuffer[i] += srcData[i];
		}
	}
	
	// 创建结果图像并求平均
	QImage result(width, height, format);
	quint16* dstData = reinterpret_cast<quint16*>(result.bits());
	int count = images.size();
	
	for (int i = 0; i < width * height; ++i)
	{
		dstData[i] = static_cast<quint16>(accumulateBuffer[i] / count);
	}
	
	qDebug() << "叠加完成：" << count << " 帧图像已累加求平均";
	return result;
}

