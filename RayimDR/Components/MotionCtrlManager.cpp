#include "MotionCtrlManager.h"

MotionCtrlManager::MotionCtrlManager(QObject* parent)
	: QObject(parent)
{}

MotionCtrlManager::~MotionCtrlManager()
{}

MotionCtrlManager& MotionCtrlManager::Instance()
{
	static MotionCtrlManager instance;
	return instance;
}

