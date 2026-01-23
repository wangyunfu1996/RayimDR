#pragma once

#include "iraydetector_global.h"

#include <QObject>
#include <QImage>
#include <QThread>
#include <QMutex>
#include <QSharedPointer>
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
	static void registerMetaTypes();  // 静态方法用于注册元类型

	bool Initialize();
	void DeInitialize();
	bool Initialized() const;
	bool CanModifyCfg() const;

	bool GetAttr(int nAttrID, int& nVal);
	bool GetAttr(int nAttrID, float& fVal);
	bool GetAttr(int nAttrID, std::string& strVal);

	bool SetAttr(int nAttrID, int nValue);
	bool SetAttr(int nAttrID, float fValue);
	bool SetAttr(int nAttrID, const char* strValue);

	bool WriteUserROM();
	bool WriteUserRAM();

	bool GetMode(std::string& mode);
	static int GetModeMaxFrameRate(std::string mode);
	static int GetMaxStackedNum();

	bool UpdateMode(std::string mode);
	bool UpdateSequenceIntervalTime(int nIntervalTime);
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
	bool GetDetectorState(int& state);

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

	void Abort();

	// 图像数据操作
	void SetReceivedImage(int width, int height, const unsigned short* pData, int nDataSize);
	QSharedPointer<QImage> GetReceivedImage() const;

	void QueryStatus();
	void StartQueryStatus();
	void StopQueryStatus();
	const NDT1717MAStatus& Status() const;

	bool CheckBatteryStateOK();

private:
	void TryOpenCorrection();

signals:
	void signalAcqImageReceived(QSharedPointer<QImage> image, int idx, int grayValue);
	void signalOffsetImageSelected(int nTotal, int nValid);
	void signalGainImageSelected(int nTotal, int nValid);
	void signalDefectImageSelected(int nTotal, int nValid);
	void signalStatusChanged();
	void signalModeChanged();
	void signalErrorOccurred(const QString& msg);

private:
	QString m_uuid;
	QString m_workDirPath;
	QSharedPointer<QImage> m_receivedImage;
	NDT1717MAStatus m_status;
	mutable QMutex m_imageDataMutex;  // 保护 m_receivedImage 的线程安全
	
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
// 声明元类型，使 QSharedPointer<QImage> 支持跨线程信号传递
Q_DECLARE_METATYPE(QSharedPointer<QImage>)