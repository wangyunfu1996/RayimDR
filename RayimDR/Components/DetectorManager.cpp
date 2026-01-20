#include "DetectorManager.h"

#include "../IRayDetector/NDT1717MA.h"

DetectorManager::DetectorManager(QObject* parent)
	: QObject(parent)
{
	DET.moveToThread(&m_thread);
	// 启动工作线程
	m_thread.start();
}

DetectorManager::~DetectorManager()
{
	// 请求线程退出
	m_thread.quit();
	// 等待线程完全结束
	m_thread.wait();
}

DetectorManager& DetectorManager::Instance()
{
	static DetectorManager instance;
	return instance;
}

void DetectorManager::Init()
{
	DET.Initialize();
}

void DetectorManager::DeInit()
{
	DET.DeInitialize();
}

// 实现异步调用，底层调用 NDT1717MA 的 Invoke
template<typename ... Args>
int DetectorManager::Invoke(int nCmdId, Args...args)
{
	return DET.Invoke(nCmdId, args...);
}

// 实现同步调用，底层调用 NDT1717MA 的 SyncInvoke
template<typename ... Args>
int DetectorManager::SyncInvoke(int nCmdId, Args...args)
{
	return DET.SyncInvoke(nCmdId, args...);
}

