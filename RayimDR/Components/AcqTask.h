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

	void doAcq(AcqCondition acqCondition);
	void stopAcq();

private:
	std::atomic_bool stopRequested{ false };
};

