#pragma once

#include <QThread>
#include <QPointer>
#include <QSharedPointer>

#include "XGlobal.h"

class AcqTask : public QThread
{
	Q_OBJECT

public:
	AcqTask(AcqCondition acqCond, QObject* parent = nullptr);
	~AcqTask();

	void startAcq();
	void stopAcq();

protected:
	virtual void run() override;

private:
	void onImageReceived(QSharedPointer<QImage> image, int idx);
	QImage stackImages(const QVector<QImage>& images);
	void processStackedFrames(const QVector<QImage>& imagesToStack);
	void onErrorOccurred(const QString& msg);
	void onProgressChanged(const QString& msg);

	AcqCondition acqCondition;
	std::atomic_bool bStopRequested{ false };
	std::atomic_int nReceivedIdx{ 0 };
	std::atomic_int nProcessedStacekd{ 0 };
};

