#pragma once

#include <QObject>
#include <QPointer>

#include "XGlobal.h"

class AcqTask : public QObject
{
	Q_OBJECT

public:
	AcqTask(QObject* parent = nullptr);
	~AcqTask();

	void startAcq(AcqCondition acqCondition);
	void stopAcq();

private:
	QImage stackImages(const QVector<QImage>& images);
	void processStackedFrames(const QVector<QImage>& imagesToStack, const AcqCondition& acqCondition, int receivedIdx);
	void onErrorOccurred(const QString& msg);
	void onProgressChanged(const QString& msg);

	std::atomic_bool stopRequested{ false };
	std::atomic_int rawFrameCount{ 0 };
	std::atomic_int receivedIdx{ 0 };
};

