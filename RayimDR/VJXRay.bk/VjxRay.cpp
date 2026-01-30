#include "VjxRay.h"
#include <QSerialPortInfo>
#include "ControllerFactory.h"
#include "XRaySourceParameter.h"
#include <QDateTime>

VjxRay::~VjxRay()
{
    if(IsStart)
    {
        this->quit();
        if(!this->wait(50))
        {
            this->terminate();
        }
    }
}
VjxRay::VjxRay():_functionIndex(0)
{
    TimeOverThreeMonth = { 300,300,300,300,300,300,300,300,300,300};
    TimeOverOneMonth = { 60,60,60,60,60,60,60,60,60,60};
    TimeOverThreeDay = { 30,30,30,30,30,30,30,30,30,30};
    //暖机电压
    WarmupVoltage = {100,110,120,130,140,150,160,170,180,200};
    //暖机电流
    WarmupCurrent = {200,261,322,383,444,506,567,628,689,750};

}
bool VjxRay::connect(XRayControllerConnectParameter & parameter)
{
    getParameter()->_isConnected = false;
	if (QSerialPortInfo::availablePorts().size() == 0)
	{
		return false;
	}
	foreach(const QSerialPortInfo & qspinfo, QSerialPortInfo::availablePorts())
	{
		qDebug() << "serialnumber:" << qspinfo.serialNumber() << " portname:" << qspinfo.portName();
	}
    //判断com口是否大于10
    QString comPort = parameter.comPort;
    QString tmpCom = comPort.toUpper();
    int tmpComLen = tmpCom.length();
    int comLen = QString("COM").length();
    if (tmpComLen > comLen)
    {
        QString num = tmpCom.right(tmpComLen - comLen);
        int iNum = num.toInt();
        if (iNum > 9)
        {
            tmpCom = "\\\\.\\" + tmpCom;
            comPort = tmpCom;
        }
    }
    //
	DCB db;
	COMMTIMEOUTS to;
    hCom = CreateFile((const wchar_t*)comPort.utf16(), GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	qDebug() << hCom;
	qDebug() << INVALID_HANDLE_VALUE;
	if (INVALID_HANDLE_VALUE == hCom)
	{
		iErr = GetLastError();
		return false;
	}
	SecureZeroMemory(&db, sizeof(DCB));
	db.DCBlength = sizeof(DCB);
	db.BaudRate = CBR_9600;											//波特率
	db.ByteSize = 8;													//数据位
	db.StopBits = ONESTOPBIT;								//停止位
	//db.fBinary = TRUE;												//二进制
	db.Parity = NOPARITY;										//无校验
	//db.fParity = FALSE;												//无校验
	if (!SetCommState(hCom, &db))
	{
		iErr = GetLastError();
		return false;
	}

	//设置消息缓冲大小
	SetupComm(hCom, 1024, 1024);
	PurgeComm(hCom, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
	//超时设置
	memset(&to, 0, sizeof(COMMTIMEOUTS));
	to.ReadIntervalTimeout = 10;
	to.ReadTotalTimeoutMultiplier = 10;
	to.WriteTotalTimeoutConstant = 1000;
	to.WriteTotalTimeoutMultiplier = 10;
	to.WriteTotalTimeoutConstant = 1000;
	if (!SetCommTimeouts(hCom, &to))
	{
		return false;
	}
	getParameter()->_isConnected = true;
	//没有的状态，默认给true
	//getParameter()->_filamentValue = 0;//因为外边判断的状态是 小于700为true，大于700为false
	getParameter()->_isCoolOk = true;
	getParameter()->_isVacuumOk = true;
	Sleep(50);
	return true;
}
void VjxRay::run()
{
	if (IsStart)
	{
		int warmUpMode = getParameter()->_warmUpMode;
		if (warmUpMode == 0)
		{
            getParameter()->_isPreHeating = false;
			return;
		}
		else if (warmUpMode == 1)
		{
            getParameter()->_isPreHeating = true;
			for (int i = 0; i < warmProcedure; i++)
			{
                getParameter()->_warmUpProgress = (int)((i*1.0f/warmProcedure)*100);
                //qDebug()<<"i="<<i<<" warupProgress="<<getParameter()->_warmUpProgress <<" i*1.0f/warmProcedure="<<i*1.0f/warmProcedure;
				setVoltageInKV(WarmupVoltage[i]);
				setCurrentInUA(WarmupCurrent[i]);
				if (!getParameter()->_isOpenXRay)
				{
					openXRay();
                    //执行预热后，则开光时间清空
                    getParameter()->_OpenXRayCmdTime = "";
				}
				int delayTime = 1000 * TimeOverThreeDay[i];
				Sleep(delayTime);
			}
            getParameter()->_warmUpProgress = 100;
		}
		else if (warmUpMode == 2)
		{
            getParameter()->_isPreHeating = true;
			for (int i = 0; i < warmProcedure; i++)
			{
                getParameter()->_warmUpProgress = (int)((i*1.0f/warmProcedure)*100);
				setVoltageInKV(WarmupVoltage[i]);
				setCurrentInUA(WarmupCurrent[i]);
				if (!getParameter()->_isOpenXRay)
				{
					openXRay();
                    //执行预热后，则开光时间清空
                    getParameter()->_OpenXRayCmdTime = "";
				}
				int delayTime = 1000 * TimeOverOneMonth[i];
				Sleep(delayTime);
			}
            getParameter()->_warmUpProgress = 100;
		}
		else if (warmUpMode == 3)
		{
            getParameter()->_isPreHeating = true;
			for (int i = 0; i < warmProcedure; i++)
			{
                getParameter()->_warmUpProgress = (int)((i*1.0f/warmProcedure)*100);
				setVoltageInKV(WarmupVoltage[i]);
				setCurrentInUA(WarmupCurrent[i]);
				if (!getParameter()->_isOpenXRay)
				{
					openXRay();
                    //执行预热后，则开光时间清空
                    getParameter()->_OpenXRayCmdTime = "";
				}
				int delayTime = 1000 * TimeOverThreeMonth[i];
				Sleep(delayTime);
			}
            getParameter()->_warmUpProgress = 100;
        }
        //为了让设置100%被界面显示，这里需要sleep一下
        msleep(5000);
        //将预热状态设置为false
        getParameter()->_isPreHeating = false;
        //关光
        closeXRay();
	}
	this->IsStart = false;
}
void VjxRay::openXRay()
{
    QString command = "\02ENBL1";
	writeCommand(command);
	QString databuff = ReadData();
    qDebug()<<"openXRay:"<<databuff;
	getParameter()->_isOpenXRay = true;
    getParameter()->_statusText = databuff;
    //每次执行开光，则自动更新开光时间
    getParameter()->_OpenXRayCmdTime = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
}

void VjxRay::closeXRay()
{
    QString command = "\02ENBL0";
	writeCommand(command);
	QString databuff = ReadData();
    qDebug() <<"closeXRay:"<<databuff;
	getParameter()->_isOpenXRay = false;
    getParameter()->_statusText = databuff;
    //执行关光后，则开光时间清空
    getParameter()->_OpenXRayCmdTime = "";
}

void VjxRay::startUp()
{
	warmUp();
}


void VjxRay::warmUp()
{
    this->IsStart = true;
	this->start();
}

bool VjxRay::setVoltageInKV(double voltage)
{
    int temp = qRound(voltage);
	QString command;
	if (temp == 0)
	{
        command = "\02VP000";
	}
	else if (temp > 0 && temp < 10)
	{
        command = "\02VP00";
	 }
	else if (temp >= 10 && temp < 100)
	{
        command = "\02VP0";
	}
	else if (temp >= 100 && temp < 1000)
	{
        command = "\02VP";
	}
    int commandVoltage = qRound(voltage*10);
    if (writeValueCommand(command, commandVoltage))
	{
		QString databuff = ReadData();
        qDebug()<<"setVoltageInKV back:"<<databuff;
		return true;
	}
    return false;
}

bool VjxRay::setPowerInWatt(double power)
{
    double current = 10;
    int voltage = getParameter()->_voltage;
    if(voltage==0)
        return false;
    if (power > m_maxPower)
    {
        power = m_maxPower;
    }
    if (power < m_minPower)
    {
        power = m_minPower;
    }
    if (voltage >= 100 && voltage <=200)
    {
        current = (power * 1000) / voltage;
        if(current>m_maxCurrent)
            current = m_maxCurrent;
        if(current<m_minCurrent)
            current = m_minCurrent;
    }

    bool ret = setCurrentInUA(current);
    return ret;
}

bool VjxRay::setCurrentInUA(double current)
{
    int val = qRound(current);
	if (val<m_minCurrent || val>m_maxCurrent)
		return false;
	int temp = val;
	QString command;
	if (temp == 0)
	{
		command = "\02CP000";
	}
	else if (temp > 0 && temp < 10)
	{
		command = "\02CP000";
	}
	else if (temp >= 10 && temp < 100)
	{
		command = "\02CP00";
	}
	else if (temp >= 100 && temp < 1000)
	{
		command = "\02CP0";
	}
    else if (temp >= 1000)
	{
		command = "\02CP";
	}
	if (writeValueCommand(command, val))
	{
		QString databuff = ReadData();
        qDebug()<<"setCurrentInUA back:"<<databuff;
		return true;
	}	
    return false;
}



bool VjxRay::resetOverload()
{
	QString command = "\02CLR";
	writeCommand(command);
	QString databuff = ReadData();
	qDebug() << databuff;
	getParameter()->_isError = false;
    getParameter()->_isOpenXRay = false;
    getParameter()->_errorText = "";
    getParameter()->_errorCode = 0;
	return true;
}

void VjxRay::setFilamentMode(int mode)
{
    Q_UNUSED(mode)
}

void VjxRay::setFocusMode(int mode)
{
    Q_UNUSED(mode)
}

void VjxRay::queryDeviceStatus()
{
    //qDebug() << "queryDeviceStatus id:"<<QThread::currentThreadId();
	switch (_functionIndex)
	{
	case 0:
		//光机参数查询
		queryParameter();
		break;
	case 1:
		//光机错误状态查询
		queryFault();
		break;
	case 2:
		//是否开机检查
		queryHVStatus();
		break;
	}
	_functionIndex = (++_functionIndex) % 3;
}

int VjxRay::getMinVoltage() const
{
	return m_minVoltage;
}

int VjxRay::getMaxVoltage()
{
	return m_maxVoltage;
}

double VjxRay::getMinPower() const
{
	return m_minPower;
}

double VjxRay::getMaxPower() 
{
	return m_maxPower;
}

int VjxRay::getMinCurrent() const
{
	return m_minCurrent;
}

int VjxRay::getMaxCurrent() const
{
	return m_maxCurrent;
}

int VjxRay::getDefaultVoltage() const
{
	return m_defaultVoltage;
}

double VjxRay::getDefaultPower() const
{
	return m_defaultPower;
}

int VjxRay::getDefaultCurrent() const
{
	return m_defaultCurrent;
}

bool VjxRay::isCurrentControl() const
{
	return m_IsCurrentControl;
}


void VjxRay::queryParameter()
{
    QString command = "\02MON";
	writeCommand(command);
	QByteArray responData = ReadData().toLatin1();
	if(responData.length()>=21)
	{
		int startIndex = 0;
		for (int i = 0; i < responData.length(); i++)
		{
			if (responData[i] == '\02')
			{
				startIndex = i;
				break;
			}
		}
		if (startIndex + 15 <= responData.length())
		{
			QString strKV = QString("%1%2%3.%4").arg(responData[startIndex + 1]).arg(responData[startIndex + 2]).arg(responData[startIndex + 3]).arg(responData[startIndex + 4]);
			QString struA = QString("%1%2%3%4").arg(responData[startIndex + 6]).arg(responData[startIndex + 7]).arg(responData[startIndex + 8]).arg(responData[startIndex + 9]);
			QString strTemp = QString("%1%2%3.%4").arg(responData[startIndex + 11]).arg(responData[startIndex + 12]).arg(responData[startIndex + 13]).arg(responData[startIndex + 14]);
			getParameter() ->_voltage = strKV.toDouble();
			getParameter()->_current = struA.toInt();
			getParameter()->_oiltemperature = strTemp.toDouble();
			//qDebug() << "_voltage:"<<getParameter()->_voltage;
		}
	}
}

void VjxRay::queryFault()
{
	QString command = "\02FLT";
	writeCommand(command);
	QByteArray responData = ReadData().toLatin1();
	QString temp_Faults = "\02";
	temp_Faults = temp_Faults + "0 0 0 0 0 0 0 0 0\r";
	if (responData.length() == temp_Faults.length())
	{
		QString strResponData = QString::fromUtf8(responData);
		int rindex = strResponData.indexOf('\r', 1);
		int index_2 = strResponData.indexOf('\02', 1);
		bool isGoodData = true;
		if (index_2 != -1 || rindex != strResponData.length() - 1)
		{
			isGoodData = false;
		}
		if (strResponData[0] == '\02' && strResponData[strResponData.length() - 1] == '\r' && isGoodData)
		{
			getParameter()->_errorText = strResponData;
			if (strResponData.contains("1"))
			{
				getParameter()->_isError = true;
			}
            else
            {
				getParameter()->_isError = false;
			}
            getParameter()->m_VJXRayErrorList._Regulation = (strResponData[17] == '0' ? false : true);
            getParameter()->m_VJXRayErrorList._interlockOpen = (strResponData[15] == '0' ? false : true);
            getParameter()->m_VJXRayErrorList._cathodeOvKV = (strResponData[13] == '0' ? false : true);
            getParameter()->m_VJXRayErrorList._anodeKOvKV = (strResponData[11] == '0' ? false : true);
            getParameter()->m_VJXRayErrorList._overTemperature = (strResponData[9] == '0' ? false : true);
            getParameter()->m_VJXRayErrorList._arc = (strResponData[7] == '0' ? false : true);
            getParameter()->m_VJXRayErrorList._overCurrent = (strResponData[5] == '0' ? false : true);
            getParameter()->m_VJXRayErrorList._powerLimit = (strResponData[3] == '0' ? false : true);
            getParameter()->m_VJXRayErrorList._overVoltage = (strResponData[1] == '0' ? false : true);
		}
	}
}

void VjxRay::queryHVStatus()
{
    QString command = "\02STAT";
	writeCommand(command);
	QByteArray responData = ReadData().toLatin1();
	if (responData.length() >= 3)
	{
		QString strState = QString("%1").arg(responData[1]);
		int iState = strState.toInt();
		if (iState == 1)
		{
			getParameter()->_isOpenXRay = true;
		}
		else
		{
			getParameter()->_isOpenXRay = false;
		}
	}
}

void VjxRay::clearFault()
{
    QString command = "\02CLR";
	writeCommand(command);
}

void VjxRay::xRayStatus()
{
    QString command = "\02STAT";
	writeCommand(command);

}

void VjxRay::queryWatchDogStatus()
{
    QString command = "\02WSTAT";
	writeCommand(command);
}

void VjxRay::watchDogSet(int IsEnable)
{
	int val = IsEnable;
    QString command = "\02WDOG";
	writeValueCommand(command, val);
	QString databuff = ReadData();
}
void parseData(QString dataBuffer)
{
}
REGISTRY_CONTROLLER(VjxRay)
