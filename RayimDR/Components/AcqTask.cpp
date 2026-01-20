#include "AcqTask.h"

#include <QtConcurrent/QtConcurrent>
#include <qdebug.h>
#include <qthread.h>
#include <qfuture.h>
#include <qdatetime.h>

#include "AcqTaskManager.h"
#include "ImageRender/XImageHelper.h"

#include "../IRayDetector/NDT1717MA.h"
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

// Helper function to process stacked frames and save results
void AcqTask::processStackedFrames(const QVector<QImage>& imagesToStack, const AcqCondition& acqCondition, int receivedIdx)
{
	int vecIdx = receivedIdx % IMAGE_BUFFER_SIZE;

	// Execute stacking in background thread
	QtConcurrent::run([this, imagesToStack, acqCondition, receivedIdx, vecIdx]() {
		QImage stackedImage = stackImages(imagesToStack);
		AcqTaskManager::Instance().receivedImageList[vecIdx] = stackedImage;
		qDebug() << "Stacking completed (background thread), receivedIdx:" << receivedIdx;

		if (acqCondition.stackedFrame > 0)
			emit AcqTaskManager::Instance().signalAcqStatusMsg("Data stacking completed", 0);

		emit AcqTaskManager::Instance().signalAcqTaskReceivedIdxChanged(acqCondition, receivedIdx);

		// Save files if specified
		if (acqCondition.saveToFiles && acqCondition.frame != INT_MAX)
		{
			if (acqCondition.saveType == ".RAW")
			{
				QString fileName = QString("%1/Image%2_%3x%4.raw")
					.arg(acqCondition.savePath)
					.arg(receivedIdx)
					.arg(DET_WIDTH)
					.arg(DET_HEIGHT);
				XImageHelper::Instance().saveImageU16Raw(stackedImage, fileName);
			}
			else if (acqCondition.saveType == ".TIFF")
			{
				QString fileName = QString("%1/Image%2.tif")
					.arg(acqCondition.savePath)
					.arg(receivedIdx);
				TiffHelper::SaveImage(stackedImage, fileName.toStdString());
			}
		}
		});
}

void AcqTask::startAcq(AcqCondition acqCondition)
{
	qDebug() << "开启采集任务，当前线程：" << QThread::currentThreadId();
	qDebug() << acqCondition;

	QPointer<AcqTask> weakThis(this);
	AcqTaskManager::Instance().stackedImageList.clear();
	rawFrameCount.store(0);
	receivedIdx.store(0);
	// Calculate total frames to stack: stackedFrame=0 means 1 frame, stackedFrame=n means 1+n frames
	int totalStackFrames = (acqCondition.stackedFrame == 0) ? 1 : (1 + acqCondition.stackedFrame);
	qDebug() << "Stack configuration: stackedFrame=" << acqCondition.stackedFrame << ", need to collect" << totalStackFrames << "frames";

#if DET_TYPE == DET_TYPE_VIRTUAL
	while (!stopRequested.load())
	{
		int currentRawCount = weakThis->rawFrameCount.fetch_add(1) + 1;
		QImage rawImage = XImageHelper::generateRandomGaussianGrayImage(DET_WIDTH, DET_HEIGHT);
		AcqTaskManager::Instance().stackedImageList.append(rawImage);
		qDebug() << "Received raw frame" << currentRawCount << ", stack buffer current frames:" << AcqTaskManager::Instance().stackedImageList.size();

		// When buffer reaches required frame count, process stacking
		if (AcqTaskManager::Instance().stackedImageList.size() == totalStackFrames)
		{
			int currentReceivedIdx = weakThis->receivedIdx.fetch_add(1) + 1;
			// Copy data and clear buffer
			QVector<QImage> imagesToStack = AcqTaskManager::Instance().stackedImageList;
			AcqTaskManager::Instance().stackedImageList.clear();

			// Process stacked frames using common helper
			processStackedFrames(imagesToStack, acqCondition, currentReceivedIdx);

			if (currentReceivedIdx == acqCondition.frame)
			{
				stopAcq();
			}
		}

		QThread::msleep(10); // Prevent high CPU usage
	}
#elif DET_TYPE == DET_TYPE_IRAY
	connect(&DET, &NDT1717MA::signalAcqImageReceived, this, [weakThis, acqCondition, totalStackFrames](int detectorFrameIdx) {
		if (!weakThis || weakThis->stopRequested.load())
		{
			qDebug() << "Received detector data, but acquisition task has been stopped";
			return;
		}

		QImage rawImage = DET.GetReceivedImage();
		AcqTaskManager::Instance().stackedImageList.append(rawImage);
		int currentRawCount = weakThis->rawFrameCount.fetch_add(1) + 1;
		qDebug() << "Received raw frame" << currentRawCount << ", stack buffer current frames:" << AcqTaskManager::Instance().stackedImageList.size();

		if (acqCondition.stackedFrame > 0)
		{
			emit AcqTaskManager::Instance().signalAcqStatusMsg(QString("叠加数据 %1/%2 已接受").arg(AcqTaskManager::Instance().stackedImageList.size()).arg(acqCondition.stackedFrame + 1), 0);
		}

		// When buffer reaches required frame count, process stacking
		if (AcqTaskManager::Instance().stackedImageList.size() == totalStackFrames)
		{
			int currentReceivedIdx = weakThis->receivedIdx.fetch_add(1) + 1;
			if (currentReceivedIdx == acqCondition.frame)
			{
				qDebug() << "Sufficient data collected, stopping acquisition";
				weakThis->stopAcq();
			}

			// Copy data and clear buffer
			QVector<QImage> imagesToStack = AcqTaskManager::Instance().stackedImageList;
			AcqTaskManager::Instance().stackedImageList.clear();

			qDebug() << "Starting async stacking, currentReceivedIdx:" << currentReceivedIdx;
			if (acqCondition.stackedFrame > 0)
				emit AcqTaskManager::Instance().signalAcqStatusMsg("开始进行数据叠加", 0);

			// Process stacked frames using common helper
			weakThis->processStackedFrames(imagesToStack, acqCondition, currentReceivedIdx);
		}
		});
	
	if (!DET.StartAcq())
	{
		emit AcqTaskManager::Instance().signalAcqStatusMsg(QString("采集失败, 请重试!"), 1);
		stopAcq();
	}
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
	qint64 startTime = QDateTime::currentMSecsSinceEpoch();

	if (images.isEmpty())
	{
		qWarning() << "叠加图像列表为空";
		return QImage();
	}

	if (images.size() == 1)
	{
		qint64 elapsedTime = QDateTime::currentMSecsSinceEpoch() - startTime;
		qDebug() << "[性能] 单帧无需叠加，耗时：" << elapsedTime << "ms";
		return images[0];
	}

	int width = images[0].width();
	int height = images[0].height();
	QImage::Format format = images[0].format();
	int count = images.size();
	int totalPixels = width * height;

	qDebug() << "[性能] 开始叠加：" << count << "帧图像，尺寸：" << width << "x" << height << "，总像素：" << totalPixels;

	qint64 accumulateStartTime = QDateTime::currentMSecsSinceEpoch();

	// 创建累加缓冲区（使用浮点数提高精度，便于并行处理）
	QVector<float> accumulateBuffer(totalPixels, 0.0f);

	// 收集所有源图像指针，减少重复访问开销
	QVector<const quint16*> pixelDataList;
	for (const QImage& img : images)
	{
		if (img.width() != width || img.height() != height || img.format() != format)
		{
			qWarning() << "叠加图像尺寸或格式不一致，跳过该帧";
			continue;
		}
		pixelDataList.append(reinterpret_cast<const quint16*>(img.constBits()));
	}

	// 【多线程优化】分块并行处理，每块64K像素
	const int BLOCK_SIZE = 65536;
	int blockCount = (totalPixels + BLOCK_SIZE - 1) / BLOCK_SIZE;

	// 并行累加所有图像
	QVector<QFuture<void>> accumulateFutures;
	for (int blockIdx = 0; blockIdx < blockCount; ++blockIdx)
	{
		auto future = QtConcurrent::run([&, blockIdx]() {
			int startIdx = blockIdx * BLOCK_SIZE;
			int endIdx = qMin(startIdx + BLOCK_SIZE, totalPixels);

			// 对这个块的所有像素进行累加
			for (const quint16* pixelData : pixelDataList)
			{
				for (int i = startIdx; i < endIdx; ++i)
				{
					accumulateBuffer[i] += static_cast<float>(pixelData[i]);
				}
			}
			});
		accumulateFutures.append(future);
	}

	// 等待所有累加任务完成
	for (auto& future : accumulateFutures)
		future.waitForFinished();

	qint64 accumulateTime = QDateTime::currentMSecsSinceEpoch() - accumulateStartTime;
	qint64 averageStartTime = QDateTime::currentMSecsSinceEpoch();

	// 创建结果图像
	QImage result(width, height, format);
	quint16* dstData = reinterpret_cast<quint16*>(result.bits());

	// 【多线程优化】并行求平均值
	float invCount = 1.0f / static_cast<float>(count);

	QVector<QFuture<void>> averageFutures;
	for (int blockIdx = 0; blockIdx < blockCount; ++blockIdx)
	{
		auto future = QtConcurrent::run([&, blockIdx, invCount]() {
			int startIdx = blockIdx * BLOCK_SIZE;
			int endIdx = qMin(startIdx + BLOCK_SIZE, totalPixels);

			for (int i = startIdx; i < endIdx; ++i)
			{
				dstData[i] = static_cast<quint16>(accumulateBuffer[i] * invCount);
			}
			});
		averageFutures.append(future);
	}

	// 等待所有平均计算任务完成
	for (auto& future : averageFutures)
		future.waitForFinished();

	qint64 averageTime = QDateTime::currentMSecsSinceEpoch() - averageStartTime;
	qint64 totalTime = QDateTime::currentMSecsSinceEpoch() - startTime;

	qDebug() << "[性能] 叠加完成：" << count << "帧图像"
		<< "，累加耗时：" << accumulateTime << "ms"
		<< "，平均耗时：" << averageTime << "ms"
		<< "，总耗时：" << totalTime << "ms"
		<< "，并行度：" << blockCount << "块"
		<< "，每像素：" << (double(totalTime) * 1000.0 / totalPixels) << "μs";

	return result;
}

