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

AcqTask::AcqTask(AcqCondition acqCond, QObject* parent)
	: QThread(parent), acqCondition(acqCond)
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
void AcqTask::processStackedFrames(const QVector<QImage>& imagesToStack)
{
	int vecIdx = nProcessedStacekd.load() % IMAGE_BUFFER_SIZE;

	// Execute stacking in background thread
	QtConcurrent::run([this, imagesToStack, vecIdx]() {
		QImage stackedImage = stackImages(imagesToStack);
		AcqTaskManager::Instance().receivedImageList[vecIdx] = stackedImage;

		if (acqCondition.stackedFrame > 0)
			onProgressChanged("数据叠加完成");

		// Save files if specified
		if (acqCondition.saveToFiles && acqCondition.frame != INT_MAX)
		{
			if (acqCondition.saveType == ".RAW")
			{
				QString fileName = QString("%1/Image%2_%3x%4.raw")
					.arg(acqCondition.savePath)
					.arg(nProcessedStacekd.load())
					.arg(stackedImage.width())
					.arg(stackedImage.height());
				XImageHelper::Instance().saveImageU16Raw(stackedImage, fileName);
			}
			else if (acqCondition.saveType == ".TIFF")
			{
				QString fileName = QString("%1/Image%2.tif")
					.arg(acqCondition.savePath)
					.arg(nProcessedStacekd.load());
				TiffHelper::SaveImage(stackedImage, fileName.toStdString());
			}
		}

		qDebug() << "数据叠加完成";
		emit AcqTaskManager::Instance().signalAcqTaskReceivedIdxChanged(acqCondition, nProcessedStacekd.load());
		nProcessedStacekd.fetch_add(1);
		});
}

void AcqTask::onErrorOccurred(const QString& msg)
{
	bStopRequested.store(true);
	emit AcqTaskManager::Instance().signalAcqStatusMsg(msg, 1);
}

void AcqTask::onProgressChanged(const QString& msg)
{
	emit AcqTaskManager::Instance().signalAcqStatusMsg(msg, 0);
}

void AcqTask::startAcq()
{
	qDebug() << "开启采集任务，当前线程：" << QThread::currentThreadId();
	qDebug() << acqCondition;

	AcqTaskManager::Instance().stackedImageList.clear();
	nReceivedIdx.store(0);
	nProcessedStacekd.store(0);
	// Calculate total frames to stack: stackedFrame=0 means 1 frame, stackedFrame=n means 1+n frames
	int totalStackFrames = (acqCondition.stackedFrame == 0) ? 1 : (1 + acqCondition.stackedFrame);
	qDebug() << "Stack configuration: stackedFrame=" << acqCondition.stackedFrame << ", need to collect" << totalStackFrames << "frames";

#if DET_TYPE == DET_TYPE_VIRTUAL
	int width{ 0 };
	int height{ 0 };
	if (acqCondition.mode == "Mode5")
	{
		width = DET_WIDTH_1X1;
		height = DET_HEIGHT_1X1;
	}
	else if (acqCondition.mode == "Mode6")
	{
		width = DET_WIDTH_2X2;
		height = DET_HEIGHT_2X2;
	}

	do
	{
		QSharedPointer<QImage> image = QSharedPointer<QImage>::create(
			XImageHelper::generateRandomGaussianGrayImage(width, height)
		);
		qDebug() << "[Virtual Mode] Calling onImageReceived with idx=" << nReceivedIdx.load();
		onImageReceived(image, nReceivedIdx.load());
		nReceivedIdx.fetch_add(1);
		QThread::msleep(1000 / acqCondition.frameRate);
	} while (!bStopRequested.load());

#elif DET_TYPE == DET_TYPE_IRAY

	// 先尝试修改探测器的工作模式和帧率等
	if (!DET.UpdateMode(acqCondition.mode))
	{
		this->onErrorOccurred("修改探测器的工作模式失败!");
		qDebug() << "修改探测器的工作模式失败!";
		return;
	}

	connect(&DET, &NDT1717MA::signalErrorOccurred, this, &AcqTask::onErrorOccurred, Qt::ConnectionType::UniqueConnection);
	bool connected = connect(&DET, &NDT1717MA::signalAcqImageReceived, this, &AcqTask::onImageReceived, Qt::ConnectionType::UniqueConnection);
	if (!connected)
	{
		qWarning() << "Failed to connect signalAcqImageReceived signal!";
	}
	else
	{
		qDebug() << "Successfully connected signalAcqImageReceived signal";
	}
	
	if (!DET.StartAcq())
	{
		this->onErrorOccurred("采集失败, 请重试!");
		qDebug() << "采集失败!";
		bStopRequested.store(true);
	}

	do
	{
		qDebug() << "等待任务线程结束";
		QThread::msleep(500);
	} while (!bStopRequested.load());

	return;
#endif // DET_TYPE
}

void AcqTask::stopAcq()
{
	bStopRequested.store(true);

#if DET_TYPE == DET_TYPE_IRAY
	DET.StopAcq();
#endif
}

void AcqTask::run()
{
	startAcq();
}

void AcqTask::onImageReceived(QSharedPointer<QImage> image, int idx)
{
	qDebug() << "Called with idx=" << idx;
	
	if (bStopRequested.load())
	{
		qDebug() << "Received detector data, but acquisition task has been stopped";
		return;
	}

	if (nProcessedStacekd.load() == acqCondition.frame)
	{
		bStopRequested.store(true);
#if DET_TYPE == DET_TYPE_VIRTUAL
#elif DET_TYPE == DET_TYPE_IRAY
		DET.StopAcq();
#endif // DET_TYPE
		qDebug() << "已采集足够数量的数据";
		return;
	}

	if (!image)
	{
		qWarning() << "Received null image pointer";
		return;
	}

	AcqTaskManager::Instance().stackedImageList.append(*image);  // 解引用指针转为 QImage
	nReceivedIdx.fetch_add(1);
	qDebug() << "Received frame" << nReceivedIdx
		<< ", stack buffer current frames:" << AcqTaskManager::Instance().stackedImageList.size()
		<< ", width:" << image->width()
		<< ", height:" << image->height()
		<< ", idxInDet:" << idx;

	if (acqCondition.stackedFrame > 0)
	{
		this->onProgressChanged(QString("第 %1 帧 叠加数据 %2/%3 已接收").arg(nProcessedStacekd.load() + 1).arg(AcqTaskManager::Instance().stackedImageList.size()).arg(acqCondition.stackedFrame + 1));
	}

	// When buffer reaches required frame count, process stacking
	if (AcqTaskManager::Instance().stackedImageList.size() == (acqCondition.stackedFrame + 1))
	{
		// Copy data and clear buffer
		QVector<QImage> imagesToStack = AcqTaskManager::Instance().stackedImageList;
		AcqTaskManager::Instance().stackedImageList.clear();

		if (acqCondition.stackedFrame > 0)
			this->onProgressChanged("开始进行数据叠加");

		// Process stacked frames using common helper
		this->processStackedFrames(imagesToStack);
	}
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
		qDebug() << "单帧无需叠加，耗时：" << elapsedTime << "ms";
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

