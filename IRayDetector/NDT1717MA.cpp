#include "NDT1717MA.h"

#include <thread>
#include <mutex>
#include <limits>

#include <qdebug.h>
#include <quuid.h>
#include <qtimer.h>
#include <QMetaObject>

#include "Common/Detector.h"
#include "Common/DisplayProgressbar.h"

#pragma warning(disable:4996)

#define dbgResult(code) qDebug() << QString::number(code) << code << " msg: " << gs_pDetInstance->GetErrorInfo(code).c_str()

const int NDT1717MA::nSuggestedKVs[nTotalGroup] = { 40, 60, 70, 80 };
const int NDT1717MA::nExpectedGrays[nTotalGroup] = { 1200, 2000, 4000, 8000 };
const int NDT1717MA::nExpectedImageCnts[nTotalGroup] = { 1, 1, 1, 5 };
const int NDT1717MA::nDefectLightExpectedValids[nTotalGroup] = { 1, 3, 5, 11 };
const int NDT1717MA::nDefectDarkExpectedValids[nTotalGroup] = { 2, 4, 6, 16 };

namespace {
	static CDetector* gs_pDetInstance = nullptr;
	static IRayTimer s_timer;
	static int s_nExpWindow = 0;

	static std::atomic_bool s_bOffsetGenerationSucceedOrFailed{ false };
	std::atomic<int> gn_receviedIdx{ 0 };
	std::atomic_bool gb_StatusQueryFlag{ false };

	void TimeProc(int uTimerID)
	{
		s_nExpWindow -= 1;
		if (0 == s_nExpWindow)
		{
			s_timer.Close();
			return;
		}
		qDebug("Please expose in %ds", s_nExpWindow);
	}

	void SDKCallbackHandler(int nDetectorID, int nEventID, int nEventLevel,
		const char* pszMsg, int nParam1, int nParam2, int nPtrParamLen, void* pParam)
	{
		qDebug() << "nEventID: " << nEventID
			<< " nEventLevel: " << nEventLevel
			<< " pszMsg: " << pszMsg
			<< " nParam1: " << nParam1
			<< " nParam2: " << nParam2
			<< " nPtrParamLen: " << nPtrParamLen
			<< " pParam: " << pParam;

		gs_pDetInstance->SDKCallback(nDetectorID, nEventID, nEventLevel, pszMsg, nParam1, nParam2, nPtrParamLen, pParam);
		switch (nEventID)
		{
		case Evt_ConnectProcess:
			break;
		case Evt_Exp_Enable:
		{
			qDebug("Prepare to expose");
			s_timer.Init(TimeProc, 1000);
			s_nExpWindow = nParam1 / 1000;
			qDebug("Please expose in %ds", s_nExpWindow);
			break;
		}
		case Evt_Image:
		{
			IRayImage* pImg = (IRayImage*)pParam;
			unsigned short* pImageData = pImg->pData;
			int nImageSize = pImg->nWidth * pImg->nHeight * pImg->nBytesPerPixel;
			int nFrameNo = gs_pDetInstance->GetImagePropertyInt(&pImg->propList, Enm_ImageTag_FrameNo);
			int nImageID = gs_pDetInstance->GetImagePropertyInt(&pImg->propList, Enm_ImageTag_ImageID);
			int nAvgValue = gs_pDetInstance->GetImagePropertyInt(&pImg->propList, Enm_ImageTag_AvgValue);
			int nCenterValue = gs_pDetInstance->GetImagePropertyInt(&pImg->propList, Enm_ImageTag_CenterValue);

			//IRayVariantMapItem* pItem = pImg->propList.pItems;
			//int nItemCnt = pImg->propList.nItemCount;
			//while (nItemCnt--)
			//{
			//	qDebug() << "pItem->nMapKey: " << pItem->nMapKey
			//		<< " Enm_ImageTag: " << QString("0x%1").arg(pItem->nMapKey, 4, 16, QChar('0')).toUpper()
			//		<< " pItem->varMapVal.vt: " << pItem->varMapVal.vt
			//		<< " pItem->varMapVal.val.nVal: " << pItem->varMapVal.val.nVal
			//		<< " pItem->varMapVal.val.fVal: " << pItem->varMapVal.val.fVal
			//		<< " pItem->varMapVal.val.strVal: " << pItem->varMapVal.val.strVal;
			//	pItem++;
			//}

			int currentTransaction = gs_pDetInstance->GetAttrInt(Attr_CurrentTransaction);
			qDebug() << "Attr_CurrentTransaction: " << currentTransaction
				<< " pImg->nWidth: " << pImg->nWidth
				<< " pImg->nHeight: " << pImg->nHeight
				<< " pImg->nBytesPerPixel: " << pImg->nBytesPerPixel
				<< " nImageSize: " << nImageSize
				<< " nFrameNo: " << nFrameNo
				<< " nImageID: " << nImageID
				<< " nAvgValue: " << nAvgValue
				<< " nCenterValue: " << nCenterValue;

			gn_receviedIdx.store(gn_receviedIdx.load() + 1);
			// 将图像数据深拷贝到QImage
			NDT1717MA::Instance().SetReceivedImage(pImg->nWidth, pImg->nHeight, pImageData, nImageSize);
			emit NDT1717MA::Instance().signalAcqImageReceived(gn_receviedIdx.load());

			if (currentTransaction == Enm_Transaction::Enm_Transaction_GainGen)
			{
				emit NDT1717MA::Instance().signalGainImageReceived(nCenterValue);
			}
			else if (currentTransaction == Enm_Transaction::Enm_Transaction_DefectGen)
			{
				emit NDT1717MA::Instance().signalDefectImageReceived(nCenterValue);
			}
			break;
		}
		case Evt_TaskResult_Succeed:
		case Evt_TaskResult_Failed:
			if (nParam1 == Cmd_OffsetGeneration)
			{
				qDebug("Offset template generated - {%s}", gs_pDetInstance->GetErrorInfo(nParam2).c_str());
				s_bOffsetGenerationSucceedOrFailed.store(true);
			}
			qDebug() << gs_pDetInstance->GetErrorInfo(nParam2).c_str();
			break;
		default:
			break;
		}
	}
}

NDT1717MA::NDT1717MA(QObject* parent)
	: QObject(parent)
{
	m_uuid = QUuid::createUuid().toString();
	qDebug() << "构造探测器实例：" << m_uuid;
}

NDT1717MA::~NDT1717MA()
{
	qDebug() << "析构探测器实例：" << m_uuid;
}


NDT1717MA& NDT1717MA::Instance()
{
	static NDT1717MA iRayDetector;
	return iRayDetector;
}

bool NDT1717MA::Initialize()
{
	if (gs_pDetInstance &&
		gs_pDetInstance->Initilized())
	{
		return true;
	}

	gs_pDetInstance = new CDetector();
	qDebug() << "Load libray";
	int result = gs_pDetInstance->LoadIRayLibrary();
	dbgResult(result);
	if (Err_OK != result)
	{
		return false;
	}

	qDebug() << "Create instance";
	result = gs_pDetInstance->Create("D:\\NDT1717MA", SDKCallbackHandler);
	dbgResult(result);
	if (Err_OK != result)
	{
		return false;
	}

	qDebug() << "Connect device";
	int timeout = 10000;
	result = gs_pDetInstance->SyncInvoke(Cmd_Connect, timeout);
	dbgResult(result);
	if (Err_OK != result)
	{
		return false;
	}

	qDebug() << "Query attrs";
	gs_pDetInstance->GetAttr(Attr_CurrentSubset, m_status.Mode);
	GetCurrentCorrectOption(m_status.SW_PreOffset, m_status.SW_Gain, m_status.SW_Defect);

	emit signalStatusChanged();
	return true;
}

void NDT1717MA::DeInitialize()
{
	if (gs_pDetInstance)
	{
		gs_pDetInstance->Destroy();
		gs_pDetInstance->FreeIRayLibrary();
		delete gs_pDetInstance;
		gs_pDetInstance = NULL;
	}
}

bool NDT1717MA::Initialized() const
{
	return (nullptr != gs_pDetInstance && gs_pDetInstance->Initilized());
}

bool NDT1717MA::CanModifyCfg() const
{
	return Initialized();
}

int NDT1717MA::GetAttr(int nAttrID, int& nVal)
{
	return gs_pDetInstance->GetAttr(nAttrID, nVal);
}

int NDT1717MA::GetAttr(int nAttrID, float& fVal)
{
	return gs_pDetInstance->GetAttr(nAttrID, fVal);
}

int NDT1717MA::GetAttr(int nAttrID, std::string& strVal)
{
	return gs_pDetInstance->GetAttr(nAttrID, strVal);
}

bool NDT1717MA::GetMode(std::string& mode)
{
	if (!Initialized())
		return false;

	int result = gs_pDetInstance->GetAttr(Attr_CurrentSubset, mode);
	dbgResult(result);
	return result == Err_OK;
}

int NDT1717MA::GetModeMaxFrameRate(std::string mode)
{
	qDebug() << "mode:" << mode.c_str();
	if (mode == "Mode5")
	{
		return 1;
	}
	else if (mode == "Mode6")
	{
		return 4;
	}
	else if (mode == "Mode7")
	{
		return 10;
	}
	else if (mode == "Mode8")
	{
		return 16;
	}
	else
	{
		return 1;
	}
}

int NDT1717MA::GetMaxStackedNum()
{
	return 10;
}

bool NDT1717MA::UpdateMode(std::string mode)
{
	std::string current_mode;
	int result = gs_pDetInstance->GetAttr(Attr_CurrentSubset, current_mode);
	dbgResult(result);
	if (current_mode == mode)
	{
		qDebug() << QStringLiteral("目标模式与当前模式相同，当前模式") << QString::fromStdString(current_mode);
		return true;
	}

	result = gs_pDetInstance->SyncInvoke(Cmd_SetCaliSubset, mode, INT_MAX);
	dbgResult(result);

	if (Err_OK != result)
	{
		qDebug() << QStringLiteral("修改探测器工作模式失败！");
		return false;
	}

	m_status.Width = gs_pDetInstance->GetAttrInt(Attr_Width);
	m_status.Height = gs_pDetInstance->GetAttrInt(Attr_Height);
	m_status.AcqParam_Binning_W = gs_pDetInstance->GetAttrInt(Attr_AcqParam_Binning_W);
	m_status.AcqParam_Zoom_W = gs_pDetInstance->GetAttrInt(Attr_AcqParam_Zoom_W);

	GetCurrentCorrectOption(m_status.SW_PreOffset, m_status.SW_Gain, m_status.SW_Defect);

	qDebug() << "修改探测器工作模式成功，"
		<< " 当前工作模式：" << QString::fromStdString(mode)
		<< " Attr_Width: " << m_status.Width
		<< " Attr_Height: " << m_status.Height
		<< " Attr_AcqParam_Binning_W: " << m_status.AcqParam_Binning_W
		<< " Attr_AcqParam_Zoom_W: " << m_status.AcqParam_Zoom_W
		<< " SW_PreOffset: " << m_status.SW_PreOffset
		<< " SW_Gain: " << m_status.SW_Gain
		<< " SW_Defect: " << m_status.SW_Defect;

	return true;
}

bool NDT1717MA::GetCurrentCorrectOption(int& sw_offset, int& sw_gain, int& sw_defect)
{
	int nCurrentCorrectOption{ -1 };
	int result = gs_pDetInstance->GetAttr(Attr_CurrentCorrectOption, nCurrentCorrectOption);
	dbgResult(result);
	if (Err_OK != result)
	{
		return false;
	}

	sw_offset = (nCurrentCorrectOption & Enm_CorrectOp_SW_PreOffset) ? 1 : 0;
	sw_gain = (nCurrentCorrectOption & Enm_CorrectOp_SW_Gain) ? 1 : 0;
	sw_defect = (nCurrentCorrectOption & Enm_CorrectOp_SW_Defect) ? 1 : 0;

	return true;
}

int NDT1717MA::SetCorrectOption(int sw_offset, int sw_gain, int sw_defect)
{
	int nCorrectOption{ Enm_CorrectOp_Null };
	if (sw_offset)
	{
		nCorrectOption |= Enm_CorrectOp_SW_PreOffset;
	}

	if (sw_gain)
	{
		nCorrectOption |= Enm_CorrectOp_SW_Gain;
	}

	if (sw_defect)
	{
		nCorrectOption |= Enm_CorrectOp_SW_Defect;
	}

	int result = gs_pDetInstance->SyncInvoke(Cmd_SetCorrectOption, nCorrectOption, INT_MAX);
	dbgResult(result);

	int CurrentCorrectOption = gs_pDetInstance->GetAttrInt(Attr_CurrentCorrectOption);

	qDebug() << " Enm_CorrectOp_SW_PreOffset: " << ((CurrentCorrectOption & Enm_CorrectOp_SW_PreOffset) ? 1 : 0)
		<< " Enm_CorrectOp_SW_PostOffset: " << ((CurrentCorrectOption & Enm_CorrectOp_SW_PostOffset) ? 1 : 0)
		<< " Enm_CorrectOp_SW_Gain: " << ((CurrentCorrectOption & Enm_CorrectOp_SW_Gain) ? 1 : 0)
		<< " Enm_CorrectOp_SW_Defect: " << ((CurrentCorrectOption & Enm_CorrectOp_SW_Defect) ? 1 : 0);

	return result == Err_OK;
}

int NDT1717MA::SetPreviewImageEnable(int enable)
{
	int current_enbale{ -1 };
	gs_pDetInstance->GetAttr(Cfg_PreviewImage_Enable, current_enbale);
	qDebug() << "Cfg_PreviewImage_Enable: " << current_enbale;
	int ret = gs_pDetInstance->SetAttr(Cfg_PreviewImage_Enable, enable);
	gs_pDetInstance->GetAttr(Cfg_PreviewImage_Enable, current_enbale);
	qDebug() << "Cfg_PreviewImage_Enable: " << current_enbale;
	return ret;
}

int NDT1717MA::SyncInvoke(int nCmdId, int timeout)
{
	qDebug() << "nCmdId: " << nCmdId
		<< " timeout: " << timeout;

	if (!gs_pDetInstance || !gs_pDetInstance->Initilized())
	{
		qWarning() << "Detector not initialized";
		return Err_NotInitialized;
	}

	int result = gs_pDetInstance->SyncInvoke(nCmdId, timeout);
	dbgResult(result);

	return result;
}

int NDT1717MA::SyncInvoke(int nCmdId, int nParam1, int timeout)
{
	qDebug() << "nCmdId: " << nCmdId
		<< " nParam1: " << nParam1
		<< " timeout: " << timeout;

	if (!gs_pDetInstance || !gs_pDetInstance->Initilized())
	{
		qWarning() << "Detector not initialized";
		return Err_NotInitialized;
	}

	int result = gs_pDetInstance->SyncInvoke(nCmdId, nParam1, timeout);
	dbgResult(result);

	return result;
}

int NDT1717MA::SyncInvoke(int nCmdId, int nParam1, int nParam2, int timeout)
{
	qDebug() << "nCmdId: " << nCmdId
		<< " nParam1: " << nParam1
		<< " nParam2: " << nParam2
		<< " timeout: " << timeout;

	if (!gs_pDetInstance || !gs_pDetInstance->Initilized())
	{
		qWarning() << "Detector not initialized";
		return Err_NotInitialized;
	}

	int result = gs_pDetInstance->SyncInvoke(nCmdId, nParam1, nParam2, timeout);
	dbgResult(result);

	return result;
}

int NDT1717MA::Invoke(int nCmdId)
{
	qDebug() << "nCmdId: " << nCmdId;

	if (!gs_pDetInstance || !gs_pDetInstance->Initilized())
	{
		qWarning() << "Detector not initialized";
		return Err_NotInitialized;
	}

	int result = gs_pDetInstance->Invoke(nCmdId);
	dbgResult(result);

	return result;
}

int NDT1717MA::Invoke(int nCmdId, int nParam1)
{
	qDebug() << "nCmdId: " << nCmdId
		<< " nParam1: " << nParam1;

	if (!gs_pDetInstance || !gs_pDetInstance->Initilized())
	{
		qWarning() << "Detector not initialized";
		return Err_NotInitialized;
	}

	int result = gs_pDetInstance->Invoke(nCmdId, nParam1);
	dbgResult(result);

	return result;
}

int NDT1717MA::Invoke(int nCmdId, int nParam1, int nParam2)
{
	qDebug() << "nCmdId: " << nCmdId
		<< " nParam1: " << nParam1
		<< " nParam2: " << nParam2;

	if (!gs_pDetInstance || !gs_pDetInstance->Initilized())
	{
		qWarning() << "Detector not initialized";
		return Err_NotInitialized;
	}

	int result = gs_pDetInstance->Invoke(nCmdId, nParam1, nParam2);
	dbgResult(result);

	return result;
}

int NDT1717MA::GetDetectorState(int& state)
{
	int result = gs_pDetInstance->GetAttr(Attr_State, state);
	dbgResult(result);

	return result;
}

void NDT1717MA::ClearAcq()
{
	if (gs_pDetInstance->GetAttrInt(Attr_State) != Enm_DetectorState::Enm_State_Ready)
	{
		qDebug() << "探测器状态未就绪";
		return;
	}

	int result = gs_pDetInstance->SyncInvoke(Cmd_ClearAcq, INT_MAX);
	dbgResult(result);

	if (result == Err_OK)
	{
		gn_receviedIdx.store(0);
	}
}

bool NDT1717MA::StartAcq()
{
	if (!CheckBatteryStateOK())
	{
		return false;
	}

	int state = Enm_State_Unknown;
	int retryTimes{ 10 };
	do
	{
		state = gs_pDetInstance->GetAttrInt(Attr_State);
		if (state != Enm_DetectorState::Enm_State_Ready)
		{
			qDebug() << "探测器状态未就绪，当前状态 " << state
				<< "Attr_CurrentTask: " << gs_pDetInstance->GetAttrInt(Attr_CurrentTask);
			std::this_thread::sleep_for(std::chrono::milliseconds(200));
			retryTimes--;
		}
	} while (state != Enm_State_Ready && retryTimes);

	if (state != Enm_State_Ready)
	{
		return Err_FPD_Busy;
	}

	int result = gs_pDetInstance->Invoke(Cmd_StartAcq);
	dbgResult(result);

	if (result == Err_TaskPending)
	{
		gn_receviedIdx.store(0);
	}

	return result == Err_TaskPending;
}

void NDT1717MA::StopAcq()
{
	int result = gs_pDetInstance->SyncInvoke(Cmd_StopAcq, 2000);
	dbgResult(result);
}

int NDT1717MA::OffsetGeneration()
{
	int nOffsetTotalFrames = gs_pDetInstance->GetAttrInt(Attr_OffsetTotalFrames);
	int nIntervalTimeOfEachFrame = gs_pDetInstance->GetAttrInt(Attr_UROM_SequenceIntervalTime);
	qDebug("Generate offset...");
	int timeout = (nOffsetTotalFrames + 10) * nIntervalTimeOfEachFrame + 5000;
	int result = gs_pDetInstance->Invoke(Cmd_OffsetGeneration);
	dbgResult(result);

	if (result == Err_TaskPending)
	{
		s_bOffsetGenerationSucceedOrFailed.store(false);
		std::thread t([this, nOffsetTotalFrames, timeout]() {
			int nValid{ 0 };
			do
			{
				nValid = gs_pDetInstance->GetAttrInt(Attr_OffsetValidFrames);
				qDebug() << "nOffsetTotalFrames: " << nOffsetTotalFrames
					<< " nValid: " << nValid;
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			} while (!s_bOffsetGenerationSucceedOrFailed.load());

			gs_pDetInstance->WaitEvent(timeout);
			qDebug() << "Generate offset done...";
			});
		t.detach();
	}

	return Err_OK;
}

bool NDT1717MA::GainInit()
{
	int result = gs_pDetInstance->SyncInvoke(Cmd_GainInit, 5000);
	dbgResult(result);
	return result == Err_OK;
}

bool NDT1717MA::GainStartAcq()
{
	if (!CheckBatteryStateOK())
	{
		return false;
	}

	int CurrentTransaction = gs_pDetInstance->GetAttrInt(Attr_CurrentTransaction);
	if (CurrentTransaction != Enm_Transaction_GainGen)
	{
		qDebug() << "当前不能进行增益矫正采集";
		return false;
	}

	int result = gs_pDetInstance->Invoke(Cmd_StartAcq);
	dbgResult(result);

	if (result == Err_TaskPending)
	{
		gn_receviedIdx.store(0);
	}

	return result == Err_TaskPending;
}

std::future<bool> NDT1717MA::GainSelectAll()
{
	int nGainTotalFrames = gs_pDetInstance->GetAttrInt(Attr_GainTotalFrames);
	int result = gs_pDetInstance->Invoke(Cmd_GainSelectAll, 0, nGainTotalFrames);
	std::future<bool> gainSelectFuture = std::async(std::launch::async, [this, nGainTotalFrames]() {
		int nValid{ 0 };
		do
		{
			nValid = gs_pDetInstance->GetAttrInt(Attr_GainValidFrames);
			qDebug() << QStringLiteral("nGainTotalFrames: ") << nGainTotalFrames
				<< QStringLiteral(" nValid: ") << nValid;

			emit signalGainImageSelected(nGainTotalFrames, nValid);
			if (gs_pDetInstance->GetAttrInt(Attr_CurrentTransaction) != Enm_Transaction_GainGen)
			{
				qDebug() << "亮场校正已退出";
				break;
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(100));

		} while (nValid < nGainTotalFrames);

		return nValid == nGainTotalFrames;
		});

	return gainSelectFuture;
}

bool NDT1717MA::GainGeneration(int timeout)
{
	int result = gs_pDetInstance->SyncInvoke(Cmd_GainGeneration, timeout);
	dbgResult(result);

	result = gs_pDetInstance->SyncInvoke(Cmd_FinishGenerationProcess, timeout);
	dbgResult(result);

	return result == Err_OK;
}

bool NDT1717MA::DefectInit()
{
	int result = gs_pDetInstance->SyncInvoke(Cmd_DefectInit, INT_MAX);
	dbgResult(result);

	return result == Err_OK || result == Err_TaskPending;
}

bool NDT1717MA::DefectStartAcq()
{
	if (!CheckBatteryStateOK())
	{
		return false;
	}

	int CurrentTransaction = gs_pDetInstance->GetAttrInt(Attr_CurrentTransaction);
	if (CurrentTransaction != Enm_Transaction_DefectGen)
	{
		qDebug() << "当前不能进行缺陷矫正采集"
			<< " Attr_CurrentTransaction: " << CurrentTransaction;
		return false;
	}

	int result = gs_pDetInstance->Invoke(Cmd_StartAcq);
	dbgResult(result);

	if (result == Err_TaskPending)
	{
		gn_receviedIdx.store(0);
	}

	return result == Err_TaskPending;
}

bool NDT1717MA::DefectSelectAll(int groupIdx)
{
	int nExpectedImageCnt = nExpectedImageCnts[groupIdx];
	int nExpectedValid = nDefectLightExpectedValids[groupIdx];
	int result = gs_pDetInstance->Invoke(Cmd_DefectSelectAll, groupIdx, nExpectedImageCnt);
	dbgResult(result);

	if (result != Err_OK &&
		result != Err_TaskPending)
	{
		return false;
	}

	int nValid{ 0 };
	std::async(std::launch::async, [this, groupIdx, nExpectedValid, &nValid]() {
		do
		{
			nValid = gs_pDetInstance->GetAttrInt(Attr_DefectValidFrames);
			qDebug() << "nExpectedValid: " << nExpectedValid
				<< " nValid: " << nValid;

			emit signalDefectImageSelected(nExpectedValid, nValid);
			if (gs_pDetInstance->GetAttrInt(Attr_CurrentTransaction) != Enm_Transaction_DefectGen)
			{
				qDebug() << "缺陷校正已退出";
				break;
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(100));

		} while (nValid < nExpectedValid);
		}
	).wait();

	qDebug() << "groupIdx: " << groupIdx
		<< " nValid: " << nValid
		<< " nExpectedValid: " << nExpectedValid;

	return nValid == nExpectedValid;
}

bool NDT1717MA::DefectForceDarkContinuousAcq(int groupIdx)
{
	int nExpectedImageCnt = nExpectedImageCnts[groupIdx];
	int nExpectedValid = nDefectDarkExpectedValids[groupIdx];
	int result = gs_pDetInstance->Invoke(Cmd_ForceDarkContinuousAcq, nExpectedImageCnt);
	dbgResult(result);

	if (result != Err_OK &&
		result != Err_TaskPending)
	{
		return false;
	}

	int nValid{ 0 };
	std::async(std::launch::async, [this, groupIdx, nExpectedValid, &nValid]() {
		do
		{
			nValid = gs_pDetInstance->GetAttrInt(Attr_DefectValidFrames);
			qDebug() << "nExpectedValid: " << nExpectedValid
				<< " nValid: " << nValid;

			emit signalDefectImageSelected(nExpectedValid, nValid);
			if (gs_pDetInstance->GetAttrInt(Attr_CurrentTransaction) != Enm_Transaction_DefectGen)
			{
				qDebug() << "缺陷校正已退出";
				break;
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(100));

		} while (nValid < nExpectedValid);
		}
	).wait();

	qDebug() << "groupIdx: " << groupIdx
		<< " nValid: " << nValid
		<< " nExpectedValid: " << nExpectedValid;

	return nValid == nExpectedValid;
}

bool NDT1717MA::DefectGeneration()
{
	int result = gs_pDetInstance->SyncInvoke(Cmd_DefectGeneration, INT_MAX);
	dbgResult(result);

	result = gs_pDetInstance->SyncInvoke(Cmd_FinishGenerationProcess, INT_MAX);
	dbgResult(result);

	return result == Err_OK;
}

int NDT1717MA::Abort()
{
	int result = gs_pDetInstance->Abort();
	dbgResult(result);
	return result;
}

void NDT1717MA::SetReceivedImage(int width, int height, const unsigned short* pData, int nDataSize)
{
	qDebug() << "进行图像拷贝，receviedIdx：" << gn_receviedIdx.load();
	// 参数验证
	if (width <= 0 || height <= 0 || !pData || nDataSize <= 0)
	{
		qWarning() << "Invalid image parameters: width=" << width
			<< " height=" << height
			<< " dataSize=" << nDataSize;
		return;
	}

	// 检查数据大小是否匹配
	int expectedSize = width * height * sizeof(unsigned short);
	if (nDataSize != expectedSize)
	{
		qWarning() << "Image data size mismatch. Expected: " << expectedSize
			<< " Actual: " << nDataSize;
		return;
	}

	// 创建16位灰度QImage
	m_receivedImage = QImage(width, height, QImage::Format_Grayscale16);

	// 深拷贝数据到QImage
	try
	{
		for (int y = 0; y < height; ++y)
		{
			// 获取当前行的指针
			ushort* destLine = reinterpret_cast<ushort*>(m_receivedImage.scanLine(y));
			if (!destLine)
			{
				qWarning() << "Failed to get scan line at row " << y;
				return;
			}

			// 计算源数据在当前行的位置
			const ushort* srcLine = pData + (y * width);

			// 拷贝一行数据
			std::copy(srcLine, srcLine + width, destLine);
		}

		qDebug() << "Image deep copied successfully: " << width << "x" << height
			<< " (" << nDataSize << " bytes)";
	}
	catch (const std::exception& e)
	{
		qWarning() << "Exception during image deep copy: " << e.what();
		m_receivedImage = QImage();  // 清空图像
	}
}

QImage NDT1717MA::GetReceivedImage() const
{
	return m_receivedImage;
}

void NDT1717MA::QueryStatus()
{
	if (!gs_pDetInstance)
	{
		return;
	}

	try
	{
		m_status.Battery_ExternalPower = gs_pDetInstance->GetAttrInt(Attr_Battery_ExternalPower);
		m_status.Battery_Exist = gs_pDetInstance->GetAttrInt(Attr_Battery_Exist);
		m_status.Battery_Remaining = gs_pDetInstance->GetAttrInt(Attr_Battery_Remaining);
		m_status.Battery_ChargingStatus = gs_pDetInstance->GetAttrInt(Attr_Battery_ChargingStatus);
		m_status.Battery_PowerWarnStatus = gs_pDetInstance->GetAttrInt(Attr_Battery_PowerWarnStatus);

		//qDebug() << "Attr_Battery_ExternalPower: " << m_status.Battery_ExternalPower
		//	<< " Attr_Battery_Exist: " << m_status.Battery_Exist
		//	<< " Attr_Battery_Remaining: " << m_status.Battery_Remaining
		//	<< " Attr_Battery_ChargingStatus: " << m_status.Battery_ChargingStatus
		//	<< " Attr_Battery_PowerWarnStatus: " << m_status.Battery_ExternalPower;
	}
	catch (const std::exception& ex)
	{
		qWarning() << "状态查询异常：" << ex.what();
	}
	catch (...)
	{
		qWarning() << "状态查询发生未知异常";
	}
}

void NDT1717MA::StartQueryStatus()
{
	if (gb_StatusQueryFlag.load()) {
		qDebug() << "状态查询已在运行中";
		return;
	}

	// 设置标志位为 true
	gb_StatusQueryFlag.store(true);

	std::thread([this]() {
		qDebug() << "状态查询线程启动";

		while (gb_StatusQueryFlag.load())
		{
			QueryStatus();
			// 每秒查询一次状态
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		}

		qDebug() << "状态查询线程退出";
		}).detach();
}

void NDT1717MA::StopQueryStatus()
{
	if (!gb_StatusQueryFlag.load()) {
		qDebug() << "状态查询未在运行";
		return;
	}

	// 设置标志位为 false
	gb_StatusQueryFlag.store(false);
}

const NDT1717MAStatus& NDT1717MA::Status() const
{
	return m_status;
}

bool NDT1717MA::CheckBatteryStateOK()
{
	QueryStatus();

	const int MIN_REMAINING = 20;
	// 存在电池，没有开启外部充电，电池点亮低于 MIN_REMAINING
	if (m_status.Battery_Exist &&
		!m_status.Battery_ExternalPower &&
		m_status.Battery_Remaining < MIN_REMAINING)
	{
		qDebug() << "电量低，不允许进行扫描";
		return false;
	}

	return true;
}
