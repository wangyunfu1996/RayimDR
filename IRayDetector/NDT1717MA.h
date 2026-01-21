#pragma once

#include "iraydetector_global.h"

#include <QObject>
#include <QImage>
#include <QThread>

#define DET NDT1717MA::Instance()

struct NDT1717MAStatus
{
	// 电池相关
	int Battery_ExternalPower{ 0 };
	int Battery_Exist{ 0 };
	int Battery_Remaining{ 0 };
	int Battery_ChargingStatus{ 0 };
	int Battery_PowerWarnStatus{ 0 };
};

class IRAYDETECTOR_EXPORT NDT1717MA : public QObject
{
	Q_OBJECT

private:
	NDT1717MA(QObject* parent = nullptr);
	~NDT1717MA();

public:
	static NDT1717MA& Instance();

	int Initialize();
	void DeInitialize();
	bool Initialized() const;
	bool CanModifyCfg() const;

	int GetAttr(int nAttrID, int& nVal);
	int GetAttr(int nAttrID, float& fVal);
	int GetAttr(int nAttrID, std::string& strVal);

	int UpdateMode(std::string mode);
	int GetCurrentCorrectOption(int& sw_offset, int& sw_gain, int& sw_defect);
	int SetCorrectOption(int sw_offset, int sw_gain, int sw_defect);
	int SetPreviewImageEnable(int enable);

	// 测试用
	int SyncInvoke(int nCmdId, int timeout);
	int SyncInvoke(int nCmdId, int nParam1, int timeout);
	int SyncInvoke(int nCmdId, int nParam1, int nParam2, int timeout);

	int Invoke(int nCmdId);
	int Invoke(int nCmdId, int nParam1);
	int Invoke(int nCmdId, int nParam1, int nParam2);

	//enum Enm_DetectorState
	//{
	//	Enm_State_Unknown = 0,
	//	Enm_State_Ready = 1,
	//	Enm_State_Busy = 2,
	//	Enm_State_Sleeping = 3,
	//};
	int GetDetectorState(int& state);

	void ClearAcq();
	bool StartAcq();
	void StopAcq();

	int OffsetGeneration();

	int GainInit();
	int GainStartAcq();
	int GainSelectAll();
	int GainGeneration(int timeout = 20000);

	int Abort();

	// 图像数据操作
	void SetReceivedImage(int width, int height, const unsigned short* pData, int nDataSize);
	QImage GetReceivedImage() const;

	void QueryStatus();
	void StartQueryStatus();
	void StopQueryStatus();

private:
	bool CheckBatteryStateOK();

signals:
	void signalAcqImageReceived(int idx);
	void signalGainImageReceived(int nCenterValue);
	void signalGaimImageSelected(int nGainTotalFrames, int nValid);

	void signalStatusChanged(const NDT1717MAStatus& status);

private:
	QString m_uuid;
	QString m_workDirPath;
	QImage m_receivedImage;
	NDT1717MAStatus m_status;
};
