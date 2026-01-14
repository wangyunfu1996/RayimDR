#pragma once

#include <QObject>

class MotionCtrlManager : public QObject
{
	Q_OBJECT

private:
	MotionCtrlManager(QObject* parent = nullptr);
	~MotionCtrlManager();

public:
	static MotionCtrlManager& Instance();
};

