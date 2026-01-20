#pragma once

#include "iraydetector_global.h"

#include <QObject>
#include <QImage>
#include <QTimer>
#include <QThread>
#include <future>
#include <mutex>

#define DET IRayDetector::Instance()

struct IRayDetectorStatus
{
	// 电池相关
	int Battery_ExternalPower{ 0 };
	int Battery_Exist{ 0 };
	int Battery_Remaining{ 0 };
	int Battery_ChargingStatus{ 0 };
	int Battery_PowerWarnStatus{ 0 };
};

class IRAYDETECTOR_EXPORT IRayDetector : public QObject
{
	Q_OBJECT

private:
	IRayDetector(QObject* parent = nullptr);
	~IRayDetector();

public:
	static IRayDetector& Instance();

	int Initialize();
	void DeInitialize();

	int GetAttr(int nAttrID, int& nVal);
	int GetAttr(int nAttrID, float& fVal);
	int GetAttr(int nAttrID, std::string& strVal);

	int UpdateMode(std::string mode);
	int GetCurrentCorrectOption(int& sw_offset, int& sw_gain, int& sw_defect);
	int SetCorrectOption(int sw_offset, int sw_gain, int sw_defect);
	int SetPreviewImageEnable(int enable);

	//enum Enm_DetectorState
	//{
	//	Enm_State_Unknown = 0,
	//	Enm_State_Ready = 1,
	//	Enm_State_Busy = 2,
	//	Enm_State_Sleeping = 3,
	//};
	int GetDetectorState(int& state);

	void ClearAcq();
	int StartAcq();
	void StopAcq();

	int OffsetGeneration();
	int GainGeneration();
	void StopGainGeneration();
	int Abort();

	// 图像数据操作
	void SetReceivedImage(int width, int height, const unsigned short* pData, int nDataSize);
	QImage GetReceivedImage() const;

	void QueryStatus();
	void StartQueryStatus();
	void StopQueryStatus();

signals:
	void signalAcqImageReceived(int idx);
	void signalStatusChanged(const IRayDetectorStatus& status);

private:
	QString m_uuid;
	QString m_workDirPath;
	QImage m_receivedImage;
	IRayDetectorStatus m_status;
};

