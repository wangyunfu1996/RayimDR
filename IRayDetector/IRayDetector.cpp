#include "IRayDetector.h"

#include <thread>

#include <qdebug.h>
#include <quuid.h>

#include "Common/Detector.h"
#include "Common/DisplayProgressbar.h"

#pragma warning(disable:4996)

namespace {
	static CDetector* gs_pDetInstance = nullptr;
	static IRayTimer s_timer;
	static int s_nExpWindow = 0;

	static std::atomic_bool s_bOffsetGenerationSucceedOrFailed{ false };

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

			qDebug() << "pImg->nWidth: " << pImg->nWidth
				<< " pImg->nHeight: " << pImg->nHeight
				<< " pImg->nBytesPerPixel: " << pImg->nBytesPerPixel
				<< " nImageSize: " << nImageSize
				<< " nFrameNo: " << nFrameNo
				<< " nImageID: " << nImageID
				<< " nAvgValue: " << nAvgValue
				<< " nCenterValue: " << nCenterValue;

			break;
		}
		case Evt_TaskResult_Succeed:
		case Evt_TaskResult_Failed:
			if (nParam1 == Cmd_OffsetGeneration)
			{
				qDebug("Offset template generated - {%s}", gs_pDetInstance->GetErrorInfo(nParam2).c_str());
				s_bOffsetGenerationSucceedOrFailed.store(true);
			}
			break;
		default:
			break;
		}
	}
}

IRayDetector::IRayDetector(QObject* parent)
	: QObject(parent)
{
	m_uuid = QUuid::createUuid().toString();
	qDebug() << "构造探测器实例：" << m_uuid;
}

IRayDetector::~IRayDetector()
{
	qDebug() << "析构探测器实例：" << m_uuid;
}


IRayDetector& IRayDetector::Instance()
{
	static IRayDetector iRayDetector;
	return iRayDetector;
}

int IRayDetector::Initialize()
{
	gs_pDetInstance = new CDetector();
	qDebug("Load libray");
	int ret = gs_pDetInstance->LoadIRayLibrary();
	if (Err_OK != ret)
	{
		qDebug("[No ]");
		return ret;
	}
	else
		qDebug("[Yes]");

	qDebug("Create instance");
	ret = gs_pDetInstance->Create("D:\\NDT1717MA", SDKCallbackHandler);
	if (Err_OK != ret)
	{
		qDebug("[No ] - error:%s", gs_pDetInstance->GetErrorInfo(ret).c_str());
		return ret;
	}
	else
		qDebug("[Yes]");

	qDebug("Connect device");
	ret = gs_pDetInstance->SyncInvoke(Cmd_Connect, 30000);
	if (Err_OK != ret)
	{
		qDebug("[No ] - error:%s", gs_pDetInstance->GetErrorInfo(ret).c_str());
		return ret;
	}
	else
		qDebug("[Yes]");

	return ret;
}

void IRayDetector::DeInitialize()
{
	if (gs_pDetInstance)
	{
		gs_pDetInstance->Destroy();
		gs_pDetInstance->FreeIRayLibrary();
		delete gs_pDetInstance;
		gs_pDetInstance = NULL;
	}
}

int IRayDetector::GetAttr(int nAttrID, int& nVal)
{
	return gs_pDetInstance->GetAttr(nAttrID, nVal);
}

int IRayDetector::GetAttr(int nAttrID, float& fVal)
{
	return gs_pDetInstance->GetAttr(nAttrID, fVal);
}

int IRayDetector::GetAttr(int nAttrID, std::string& strVal)
{
	return gs_pDetInstance->GetAttr(nAttrID, strVal);
}

int IRayDetector::UpdateMode(std::string mode)
{
	std::string current_mode;
	int ret = gs_pDetInstance->GetAttr(Attr_CurrentSubset, current_mode);
	if (current_mode == mode)
	{
		qDebug() << "目标模式与当前模式相同，当前模式" << current_mode.c_str();
		return Err_OK;
	}

	ret = gs_pDetInstance->SyncInvoke(Cmd_SetCaliSubset, mode, 50000);
	if (Err_OK != ret)
	{
		qDebug() << "修改探测器工作模式失败！";
		return ret;
	}

	int w{ -1 };
	gs_pDetInstance->GetAttr(Attr_Width, w);
	int h{ -1 };
	gs_pDetInstance->GetAttr(Attr_Height, h);
	int bin{ -1 };
	gs_pDetInstance->GetAttr(Attr_AcqParam_Binning_W, bin);
	int zoom{ -1 };
	gs_pDetInstance->GetAttr(Attr_AcqParam_Zoom_W, zoom);

	qDebug() << "修改探测器工作模式成功，"
		<< " 当前工作模式：" << mode.c_str()
		<< " Attr_Width: " << w
		<< " Attr_Height: " << h
		<< " Attr_AcqParam_Binning_W: " << bin
		<< " Attr_AcqParam_Zoom_W: " << zoom;

	return Err_OK;
}

int IRayDetector::GetCurrentCorrectOption(int& sw_offset, int& sw_gain, int& sw_defect)
{
	if (nullptr == gs_pDetInstance)
	{
		return Err_NotInitialized;
	}

	int nCurrentCorrectOption{ -1 };
	int ret = gs_pDetInstance->GetAttr(Attr_CurrentCorrectOption, nCurrentCorrectOption);
	if (Err_OK != ret)
	{
		return ret;
	}

	sw_offset = (nCurrentCorrectOption & Enm_CorrectOp_SW_PreOffset) ? 1 : 0;
	sw_gain = (nCurrentCorrectOption & Enm_CorrectOp_SW_Gain) ? 1 : 0;
	sw_defect = (nCurrentCorrectOption & Enm_CorrectOp_SW_Defect) ? 1 : 0;

	return Err_OK;
}

int IRayDetector::SetCorrectOption(int sw_offset, int sw_gain, int sw_defect)
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

	int ret = gs_pDetInstance->SyncInvoke(Cmd_SetCorrectOption, nCorrectOption, 5000);
	if (Err_OK != ret)
	{
		qDebug() << "修改探测器校正模式失败！"
			<< gs_pDetInstance->GetErrorInfo(ret).c_str();
		return ret;
	}

	qDebug() << "修改探测器校正模式成功："
		<< " Enm_CorrectOp_SW_PreOffset: " << sw_offset
		<< " Enm_CorrectOp_SW_Gain: " << sw_gain
		<< " Enm_CorrectOp_SW_Defect: " << sw_defect;
}

int IRayDetector::SetPreviewImageEnable(int enable)
{
	{
		int current_enbale{ -1 };
		gs_pDetInstance->GetAttr(Cfg_PreviewImage_Enable, current_enbale);
		qDebug() << "Cfg_PreviewImage_Enable: " << current_enbale;
	}

	int ret = gs_pDetInstance->SetAttr(Cfg_PreviewImage_Enable, enable);

	{
		int current_enbale{ -1 };
		gs_pDetInstance->GetAttr(Cfg_PreviewImage_Enable, current_enbale);
		qDebug() << "Cfg_PreviewImage_Enable: " << current_enbale;
	}
	return ret;
}

int IRayDetector::GetDetectorState(int& state)
{
	int result = gs_pDetInstance->GetAttr(Cfg_PreviewImage_Enable, state);
	qDebug() << "result: " << result
		<< gs_pDetInstance->GetErrorInfo(result).c_str();
	return result;
}

void IRayDetector::ClearAcq()
{
	int result = gs_pDetInstance->SyncInvoke(Cmd_ClearAcq, 10000);
	qDebug() << "result: " << result
		<< gs_pDetInstance->GetErrorInfo(result).c_str();
}

void IRayDetector::StartAcq()
{
	qDebug("Sequence acquiring...");
	int result = gs_pDetInstance->Invoke(Cmd_StartAcq);
	qDebug() << "result: " << result
		<< gs_pDetInstance->GetErrorInfo(result).c_str();
}

void IRayDetector::StopAcq()
{
	qDebug("Stop Sequence acquiring...");
	gs_pDetInstance->SyncInvoke(Cmd_StopAcq, 2000);
}

int IRayDetector::OffsetGeneration()
{
	int nOffsetTotalFrames = gs_pDetInstance->GetAttrInt(Attr_OffsetTotalFrames);
	int nIntervalTimeOfEachFrame = gs_pDetInstance->GetAttrInt(Attr_UROM_SequenceIntervalTime);
	qDebug("Generate offset...");
	int timeout = (nOffsetTotalFrames + 10) * nIntervalTimeOfEachFrame + 5000;
	int ret = gs_pDetInstance->Invoke(Cmd_OffsetGeneration);
	if (ret == Err_TaskPending)
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

int IRayDetector::GainGeneration()
{
	qDebug("Generate gain...");
	int result = gs_pDetInstance->SyncInvoke(Cmd_GainInit, 5000);
	if (Err_OK != result)
	{
		qDebug("GainInit failed! err=%s", gs_pDetInstance->GetErrorInfo(result).c_str());
		return result;
	}

	// 获取参数
	int nGainTotalFrames = gs_pDetInstance->GetAttrInt(Attr_GainTotalFrames);

	// 等待射线源就绪

	// 采集亮场图像
	result = gs_pDetInstance->Invoke(Cmd_StartAcq);
	if (result == Err_TaskPending)
	{
		std::thread t([this, nGainTotalFrames]() {
			int nValid{ 0 };
			do
			{
				nValid = gs_pDetInstance->GetAttrInt(Attr_GainValidFrames);
				qDebug() << "nGainTotalFrames: " << nGainTotalFrames
					<< " nValid: " << nValid;
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			} while (nValid < nGainTotalFrames);

			int ret = gs_pDetInstance->Invoke(Cmd_GainGeneration);
			if (ret != Err_OK)
			{
				qDebug("Generate gain map failed! err=%s", gs_pDetInstance->GetErrorInfo(ret).c_str());
			}
			else
			{
				qDebug() << "Generate gain done...";
			}
			});
		t.detach();
	}
	return Err_OK;
}

void IRayDetector::StopGainGeneration()
{
	int result = gs_pDetInstance->SyncInvoke(Cmd_FinishGenerationProcess, 3000);
	qDebug() << "result: " << result
		<< gs_pDetInstance->GetErrorInfo(result).c_str();
}

int IRayDetector::Abort()
{
	int result = gs_pDetInstance->Abort();
	qDebug() << "result: " << result
		<< gs_pDetInstance->GetErrorInfo(result).c_str();
	return result;
}


