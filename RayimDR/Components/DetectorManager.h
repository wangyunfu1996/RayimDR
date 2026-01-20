#pragma once

#include <QObject>
#include <QThread>

class DetectorManager : public QObject
{
	Q_OBJECT

private:
	DetectorManager(QObject* parent = nullptr);
	~DetectorManager();

public:
	static DetectorManager& Instance();

	void Init();
	void DeInit();

	// 异步调用探测器命令
	template<typename ... Args>
	int Invoke(int nCmdId, Args...args);

	// 同步调用探测器命令
	template<typename ... Args>
	int SyncInvoke(int nCmdId, Args...args);

private:
	QThread m_thread;
};

