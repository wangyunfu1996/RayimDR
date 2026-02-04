#include "AcqTask.h"

#include <QtConcurrent/QtConcurrent>
#include <qdebug.h>
#include <qthread.h>
#include <qfuture.h>
#include <qdatetime.h>
#include <qelapsedtimer.h>
#include <qrandom.h>

#include "AcqTaskManager.h"
#include "ImageRender/XImageHelper.h"

#include "IRayDetector/NDT1717MA.h"
#include "IRayDetector/TiffHelper.h"

AcqTask::AcqTask(AcqCondition acqCond, QObject* parent) : QThread(parent), acqCondition(acqCond)
{
    qDebug() << "[构造] 创建采集任务对象 - 采集模式:" << (acqCond.acqType == AcqType::DR ? "DR" : "其他") << ", 帧数:" << acqCond.frame << ", 帧率:" << acqCond.frameRate << "fps";
}

AcqTask::~AcqTask()
{
    qDebug() << "[析构] 采集任务清理开始...";
    
    disconnect(&DET, nullptr, nullptr, nullptr);    
    AcqTaskManager::Instance().stackedImageList.clear();
}

// Helper function to process stacked frames and save results
void AcqTask::processStackedFrames(const QVector<QImage>& imagesToStack)
{
    int vecIdx = nProcessedStacekd.load() % IMAGE_BUFFER_SIZE;
    qint64 processStartTime = QDateTime::currentMSecsSinceEpoch();

    qDebug() << "[处理叠加] 第" << (nProcessedStacekd.load() + 1) << "组数据, 缓冲索引:" << vecIdx << ", 帧数:" << imagesToStack.size();

    // Execute stacking in background thread
    QtConcurrent::run(
        [this, imagesToStack, vecIdx, processStartTime]()
        {
            QImage stackedImage = stackImages(imagesToStack);
            
            if (stackedImage.isNull())
            {
                qCritical() << "[异步处理] 叠加结果为空";
                return;
            }

            AcqTaskManager::Instance().receivedImageList[vecIdx] = stackedImage;
            qDebug() << "[异步处理] 叠加完成, 结果尺寸:" << stackedImage.width() << "x" << stackedImage.height() << ", 缓冲索引:" << vecIdx;

            if (acqCondition.stackedFrame > 0)
                onProgressChanged("数据叠加完成");

            // Save files if specified
            if (acqCondition.saveToFiles && acqCondition.frame != INT_MAX)
            {
                qint64 saveStartTime = QDateTime::currentMSecsSinceEpoch();
                
                try
                {
                    if (acqCondition.saveType == ".RAW")
                    {
                        QString fileName = QString("%1/Image%2_%3x%4.raw")
                                               .arg(acqCondition.savePath)
                                               .arg(nProcessedStacekd.load())
                                               .arg(stackedImage.width())
                                               .arg(stackedImage.height());
                        
                        XImageHelper::Instance().saveImageU16Raw(stackedImage, fileName);
                        qint64 saveTime = QDateTime::currentMSecsSinceEpoch() - saveStartTime;
                        qDebug() << "[文件保存] RAW文件保存成功, 文件:" << fileName << ", 耗时:" << saveTime << "ms";
                    }
                    else if (acqCondition.saveType == ".TIFF")
                    {
                        QString fileName = QString("%1/Image%2.tif").arg(acqCondition.savePath).arg(nProcessedStacekd.load());
                        TiffHelper::SaveImage(stackedImage, fileName.toStdString());
                        qint64 saveTime = QDateTime::currentMSecsSinceEpoch() - saveStartTime;
                        qDebug() << "[文件保存] TIFF文件保存成功, 文件:" << fileName << ", 耗时:" << saveTime << "ms";
                    }
                    else
                    {
                        qWarning() << "[文件保存] 未知的保存格式:" << acqCondition.saveType;
                    }
                }
                catch (const std::exception& e)
                {
                    qCritical() << "[文件保存] 文件保存异常:" << e.what();
                }
            }

            qint64 totalProcessTime = QDateTime::currentMSecsSinceEpoch() - processStartTime;
            qDebug() << "[异步处理] 第" << (nProcessedStacekd.load() + 1) << "组数据处理完成, 耗时:" << totalProcessTime << "ms";
            
            emit AcqTaskManager::Instance().signalAcqTaskReceivedIdxChanged(acqCondition, nProcessedStacekd.load());
            nProcessedStacekd.fetch_add(1);
        });
}

void AcqTask::onErrorOccurred(const QString& msg)
{
    qCritical() << "[错误] 采集失败 - 消息:" << msg << ", 时间:" << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz") << ", 线程:" << QThread::currentThreadId() << ", 已接收:" << nReceivedIdx.load() << ", 已处理:" << nProcessedStacekd.load() << ", 缓冲大小:" << AcqTaskManager::Instance().stackedImageList.size();

    bStopRequested.store(true);
    emit AcqTaskManager::Instance().signalAcqErr(msg);
}

void AcqTask::onProgressChanged(const QString& msg)
{
    static QElapsedTimer lastLogTime;
    if (!lastLogTime.isValid() || lastLogTime.elapsed() > 1000)  // 至多每秒输出一次
    {
        qInfo() << "[进度] " << msg
                << "(" << nReceivedIdx.load() << "/" << acqCondition.frame << ")";
        lastLogTime.start();
    }
    emit AcqTaskManager::Instance().signalAcqProgressChanged(msg);
}

void AcqTask::startAcq()
{
    qDebug() << "[采集开始] 线程:" << QThread::currentThreadId() << ", 时间:" << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");

    qDebug() << "[采集参数] 模式:" << (acqCondition.acqType == AcqType::DR ? "DR" : "其他") << ", 工作模式:" << acqCondition.mode.c_str() << ", 帧数:" << acqCondition.frame << (acqCondition.frame == INT_MAX ? " (连续)" : "") << ", 帧率:" << acqCondition.frameRate << "fps, 叠加:" << acqCondition.stackedFrame << ", 电压:" << acqCondition.voltage << "kV, 电流:" << acqCondition.current << "mA, 保存:" << (acqCondition.saveToFiles ? "是" : "否");

    if (acqCondition.saveToFiles)
    {
        qDebug() << "[保存配置] 路径:" << acqCondition.savePath << ", 格式:" << acqCondition.saveType;
    }

    AcqTaskManager::Instance().stackedImageList.clear();
    nReceivedIdx.store(0);
    nProcessedStacekd.store(0);
    bStopRequested.store(false);

    int totalStackFrames = (acqCondition.stackedFrame == 0) ? 1 : (1 + acqCondition.stackedFrame);
    qDebug() << "[初始化] 堆栈配置: 需要采集" << totalStackFrames << "帧进行叠加";

#if DET_TYPE == DET_TYPE_VIRTUAL
    int width{0};
    int height{0};
    if (acqCondition.mode == "Mode5")
    {
        width = DET_WIDTH_1X1;
        height = DET_HEIGHT_1X1;
        qDebug() << "[虚拟模式] 检测器: 1X1, 分辨率:" << width << "x" << height;
    }
    else if (acqCondition.mode == "Mode6")
    {
        width = DET_WIDTH_2X2;
        height = DET_HEIGHT_2X2;
        qDebug() << "[虚拟模式] 检测器: 2X2, 分辨率:" << width << "x" << height;
    }
    else
    {
        qWarning() << "[虚拟模式] 未知模式:" << acqCondition.mode.c_str();
    }

    qDebug() << "[虚拟模式] 预期帧数:" << acqCondition.frame << ", 帧间隔:" << (1000 / acqCondition.frameRate) << "ms, 开始采集...";

    int frameCount = 0;
    do
    {
        if (frameCount % 10 == 0)
        {
            qDebug() << "[虚拟采集] 进度: 已采集" << frameCount << "帧";
        }

        QSharedPointer<QImage> image =
            QSharedPointer<QImage>::create(XImageHelper::generateRandomGaussianGrayImage(width, height));
        int grayValue = 400 + QRandomGenerator::global()->bounded(100);
        
        onImageReceived(image, nReceivedIdx.load(), grayValue);
        nReceivedIdx.fetch_add(1);
        frameCount++;
        
        QThread::msleep(1000 / acqCondition.frameRate);
    } while (!bStopRequested.load());

    qDebug() << "[虚拟采集] 完成, 总帧数:" << frameCount;

#elif DET_TYPE == DET_TYPE_IRAY
    qDebug() << "[硬件采集] 准备启动, 修改工作模式为:" << acqCondition.mode.c_str();
    
    if (!DET.UpdateMode(acqCondition.mode))
    {
        QString errMsg = "修改探测器的工作模式失败";
        qCritical() << "[硬件采集] 失败:" << errMsg;
        this->onErrorOccurred(errMsg);
        return;
    }
    qDebug() << "[硬件采集] 工作模式修改成功";

    disconnect(&DET, nullptr, nullptr, nullptr);
    
    connect(&DET, &NDT1717MA::signalErrorOccurred, this, &AcqTask::onErrorOccurred,
            Qt::ConnectionType::UniqueConnection);
    
    bool connected = connect(&DET, &NDT1717MA::signalAcqImageReceived, this, &AcqTask::onImageReceived,
                             Qt::ConnectionType::UniqueConnection);
    if (!connected)
    {
        qCritical() << "[硬件采集] 连接signalAcqImageReceived失败";
    }
    else
    {
        qDebug() << "[硬件采集] 探测器信号连接成功";
    }

    qint64 acqStartTime = QDateTime::currentMSecsSinceEpoch();
    qDebug() << "[硬件采集] 启动采集...";
    
    if (!DET.StartAcq())
    {
        QString errMsg = "采集失败, 请重试";
        qCritical() << "[硬件采集] 启动失败:" << errMsg;
        this->onErrorOccurred(errMsg);
        bStopRequested.store(true);
        return;
    }
    qDebug() << "[硬件采集] 采集已启动";

    int checkCount = 0;
    do
    {
        checkCount++;
        if (checkCount % 10 == 0)
        {
            qDebug() << "[硬件采集] 进度 - 已接收:" << nReceivedIdx.load() << "帧, 已处理:" << nProcessedStacekd.load() << "帧";
        }
        QThread::msleep(500);
    } while (!bStopRequested.load());

    qint64 acqEndTime = QDateTime::currentMSecsSinceEpoch();
    qDebug() << "[硬件采集] 完成, 耗时:" << (acqEndTime - acqStartTime) << "ms, 接收:" << nReceivedIdx.load() << "帧, 处理:" << nProcessedStacekd.load() << "帧";

    return;
#endif
}

void AcqTask::stopAcq()
{
    qDebug() << "[停止采集] 时间:" << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz") << ", 接收:" << nReceivedIdx.load() << "帧, 处理:" << nProcessedStacekd.load() << "帧, 缓冲大小:" << AcqTaskManager::Instance().stackedImageList.size();
    
    bStopRequested.store(true);

#if DET_TYPE == DET_TYPE_IRAY
    DET.StopAcq();
    qDebug() << "[停止采集] 硬件采集已停止";
#endif
}

void AcqTask::run()
{
    startAcq();
}

void AcqTask::onImageReceived(QSharedPointer<QImage> image, int idx, int grayValue)
{
    if (bStopRequested.load())
    {
        qDebug() << "[接收] idx=" << idx << ", 采集已停止, 忽略此帧";
        return;
    }

    if (nProcessedStacekd.load() == acqCondition.frame)
    {
        qDebug() << "[接收] idx=" << idx << ", 已采集足够数据, 处理数:" << nProcessedStacekd.load() << ", 停止采集";
        bStopRequested.store(true);
#if DET_TYPE == DET_TYPE_VIRTUAL
#elif DET_TYPE == DET_TYPE_IRAY
        DET.StopAcq();
#endif
        return;
    }

    if (!image)
    {
        qCritical() << "[接收] idx=" << idx << ", 接收到空指针";
        return;
    }

    if (image->width() <= 0 || image->height() <= 0)
    {
        qWarning() << "[接收] idx=" << idx << ", 图像尺寸异常:" << image->width() << "x" << image->height();
        return;
    }

    AcqTaskManager::Instance().stackedImageList.append(*image);
    nReceivedIdx.fetch_add(1);

    int currentBufferSize = AcqTaskManager::Instance().stackedImageList.size();
    int expectedStackCount = acqCondition.stackedFrame + 1;

    // 定期输出接收进度
    if (nReceivedIdx.load() % 10 == 0 || currentBufferSize == 1)
    {
        qDebug() << "[接收] idx=" << idx << ", 缓冲:" << currentBufferSize << "/" << expectedStackCount << ", 尺寸:" << image->width() << "x" << image->height() << ", 灰度:" << grayValue << ", 累计:" << nReceivedIdx.load() << "帧";
    }

    if (acqCondition.stackedFrame > 0)
    {
        this->onProgressChanged(QString("第 %1 帧 叠加数据 %2/%3 已接收")
                                    .arg(nProcessedStacekd.load() + 1)
                                    .arg(currentBufferSize)
                                    .arg(expectedStackCount));
    }

    // 当缓冲区满足叠加要求时处理
    if (currentBufferSize == expectedStackCount)
    {
        qDebug() << "[缓冲区满] 达到叠加要求, 准备处理" << currentBufferSize << "帧数据";
        
        QVector<QImage> imagesToStack = AcqTaskManager::Instance().stackedImageList;
        AcqTaskManager::Instance().stackedImageList.clear();

        if (acqCondition.stackedFrame > 0)
        {
            qDebug() << "[叠加] 开始数据叠加, 帧数:" << imagesToStack.size();
            this->onProgressChanged("开始进行数据叠加");
        }

        this->processStackedFrames(imagesToStack);
    }
}

QImage AcqTask::stackImages(const QVector<QImage>& images)
{
    qint64 startTime = QDateTime::currentMSecsSinceEpoch();

    if (images.isEmpty())
    {
        qCritical() << "[叠加] 图像列表为空";
        return QImage();
    }

    if (images.size() == 1)
    {
        qint64 elapsedTime = QDateTime::currentMSecsSinceEpoch() - startTime;
        qDebug() << "[叠加] 单帧无需叠加, 直接返回, 耗时:" << elapsedTime << "ms";
        return images[0];
    }

    int width = images[0].width();
    int height = images[0].height();
    QImage::Format format = images[0].format();
    int count = images.size();
    int totalPixels = width * height;
    qint64 totalMemory = totalPixels * sizeof(float) + totalPixels * sizeof(quint16) * count;

    qDebug() << "[叠加] 开始处理, 帧数:" << count << ", 分辨率:" << width << "x" << height << " (" << totalPixels << "像素), 格式:" << (int)format << ", 内存:" << (totalMemory / 1024.0 / 1024.0) << "MB";

    qint64 accumulateStartTime = QDateTime::currentMSecsSinceEpoch();

    QVector<float> accumulateBuffer(totalPixels, 0.0f);
    qDebug() << "[叠加] 缓冲区分配成功";

    QVector<const quint16*> pixelDataList;
    int validFrames = 0;
    
    for (int i = 0; i < images.size(); ++i)
    {
        const QImage& img = images[i];
        if (img.width() != width || img.height() != height || img.format() != format)
        {
            qWarning() << "[叠加] 第" << i << "帧尺寸或格式不一致: " << img.width() << "x" << img.height() << " (格式:" << (int)img.format() << "), 跳过";
            continue;
        }
        pixelDataList.append(reinterpret_cast<const quint16*>(img.constBits()));
        validFrames++;
    }
    qDebug() << "[叠加] 有效帧数:" << validFrames << "/" << count;

    const int BLOCK_SIZE = 65536;
    int blockCount = (totalPixels + BLOCK_SIZE - 1) / BLOCK_SIZE;
    qDebug() << "[叠加] 并行配置 - 块大小:" << BLOCK_SIZE << "像素, 块数:" << blockCount << ", 开始累加...";

    qint64 accumulatePhaseStart = QDateTime::currentMSecsSinceEpoch();

    QVector<QFuture<void>> accumulateFutures;
    for (int blockIdx = 0; blockIdx < blockCount; ++blockIdx)
    {
        auto future = QtConcurrent::run(
            [&, blockIdx]()
            {
                int startIdx = blockIdx * BLOCK_SIZE;
                int endIdx = qMin(startIdx + BLOCK_SIZE, totalPixels);

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

    for (auto& future : accumulateFutures)
        future.waitForFinished();

    qint64 accumulateTime = QDateTime::currentMSecsSinceEpoch() - accumulatePhaseStart;
    qDebug() << "[叠加] 累加完成, 耗时:" << accumulateTime << "ms";

    qint64 averageStartTime = QDateTime::currentMSecsSinceEpoch();

    QImage result(width, height, format);
    quint16* dstData = reinterpret_cast<quint16*>(result.bits());
    float invCount = 1.0f / static_cast<float>(validFrames);

    QVector<QFuture<void>> averageFutures;
    for (int blockIdx = 0; blockIdx < blockCount; ++blockIdx)
    {
        auto future = QtConcurrent::run(
            [&, blockIdx, invCount]()
            {
                int startIdx = blockIdx * BLOCK_SIZE;
                int endIdx = qMin(startIdx + BLOCK_SIZE, totalPixels);

                for (int i = startIdx; i < endIdx; ++i)
                {
                    dstData[i] = static_cast<quint16>(accumulateBuffer[i] * invCount);
                }
            });
        averageFutures.append(future);
    }

    for (auto& future : averageFutures)
        future.waitForFinished();

    qint64 averageTime = QDateTime::currentMSecsSinceEpoch() - averageStartTime;
    qint64 totalTime = QDateTime::currentMSecsSinceEpoch() - startTime;

    qDebug() << "[叠加] 完成 - 总耗时:" << totalTime << "ms, 累加:" << accumulateTime << "ms (" << (100.0 * accumulateTime / totalTime) << "%), 平均:" << averageTime << "ms (" << (100.0 * averageTime / totalTime) << "%), 并行度:" << blockCount << "块, 吞吐量:" << (totalPixels / (totalTime / 1000.0) / 1e6) << "MPixels/s, 每像素:" << (double(totalTime) * 1000.0 / totalPixels) << "us";

    return result;
}
