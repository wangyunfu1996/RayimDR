#pragma once

#include "iraydetector_global.h"

#include <QObject>
#include <QImage>

#define DET IRayDetector::Instance()

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
	void StartAcq();
	void StopAcq();

	int OffsetGeneration();
	int GainGeneration();
	void StopGainGeneration();
	int Abort();

	// 图像数据操作
	void setReceivedImage(int width, int height, const unsigned short* pData, int nDataSize);
	QImage getReceivedImage() const;

signals:
	void signalAcqImageReceived(int idx);

private:
	QString m_uuid;
	QString m_workDirPath;
	QImage m_receivedImage;
};

