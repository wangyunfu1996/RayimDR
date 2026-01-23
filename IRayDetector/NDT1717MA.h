#pragma once

#include "iraydetector_global.h"

#include <QObject>
#include <QImage>
#include <QThread>
#include <future>

#define DET NDT1717MA::Instance()

struct NDT1717MAStatus
{
	// 电池相关
	int Battery_ExternalPower{ 0 };
	int Battery_Exist{ 0 };
	int Battery_Remaining{ 0 };
	int Battery_ChargingStatus{ 0 };
	int Battery_PowerWarnStatus{ 0 };

	int SW_PreOffset{ 0 };
	int SW_Gain{ 0 };
	int SW_Defect{ 0 };

	bool connected{ false };

	std::string Mode{ "Mode5" };
	int Max_FrameRate{ 1 };
	int Width{ 0 };
	int Height{ 0 };
	int AcqParam_Binning_W{ 0 };
	int AcqParam_Zoom_W{ 0 };
	
};

class IRAYDETECTOR_EXPORT NDT1717MA : public QObject
{
	Q_OBJECT

private:
	NDT1717MA(QObject* parent = nullptr);
	~NDT1717MA();

public:
	static NDT1717MA& Instance();

	bool Initialize();
	void DeInitialize();
	bool Initialized() const;
	bool CanModifyCfg() const;

	int GetAttr(int nAttrID, int& nVal);
	int GetAttr(int nAttrID, float& fVal);
	int GetAttr(int nAttrID, std::string& strVal);
	bool GetMode(std::string& mode);
	static int GetModeMaxFrameRate(std::string mode);
	static int GetMaxStackedNum();

	bool UpdateMode(std::string mode);
	bool GetCurrentCorrectOption(int& sw_offset, int& sw_gain, int& sw_defect);
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

	bool OffsetGeneration();
	bool OffsetValid();

	bool GainInit();
	bool GainStartAcq();
	std::future<bool> GainSelectAll();
	bool GainGeneration(int timeout = 20000);
	bool GainValid();

	bool DefectInit();
	bool DefectStartAcq();
	bool DefectSelectAll(int groupIdx);
	bool DefectForceDarkContinuousAcq(int groupIdx);
	bool DefectGeneration();
	bool DefectValid();

	int Abort();

	// 图像数据操作
	void SetReceivedImage(int width, int height, const unsigned short* pData, int nDataSize, int nGray);
	QImage GetReceivedImage() const;
	int GetGrayOfReceivedImage() const;

	void QueryStatus();
	void StartQueryStatus();
	void StopQueryStatus();
	const NDT1717MAStatus& Status() const;

	bool CheckBatteryStateOK();

signals:
	void signalAcqImageReceived(int idx);
	void signalOffsetImageSelected(int nTotal, int nValid);
	void signalGainImageSelected(int nTotal, int nValid);
	void signalDefectImageSelected(int nTotal, int nValid);
	void signalStatusChanged();
	void signalModeChanged();

private:
	QString m_uuid;
	QString m_workDirPath;
	QImage m_receivedImage;
	int m_nGray{ 0 };
	NDT1717MAStatus m_status;
	
public:
	const static int nTotalGroup{ 4 };
	const static int nDefectVoltages[nTotalGroup];
	const static int nDefectBeginCurrents[nTotalGroup];
	const static int nDefectExpectedGrays[nTotalGroup];
	const static int nDefectExpectedImageCnts[nTotalGroup];
	const static int nDefectLightExpectedValids[nTotalGroup];
	const static int nDefectDarkExpectedValids[nTotalGroup];

	const static int nGainVoltage{ 70 };
	const static int nGainExpectedGray{ 12000 };
	const static int nGainBeginCurrent{ 100 };
};
