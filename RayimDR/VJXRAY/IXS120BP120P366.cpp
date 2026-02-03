#include "IXS120BP120P366.h"
#include <QDebug>
#include <QMetaObject>

#include "TcpClient.h"

#pragma warning(disable : 4996)  // disable deprecated function warning

namespace
{
const int CMD_MAX_LENGTH = 48;
const std::string CMD_PREFIX = "\x02";  // STX
const std::string CMD_SUFFIX = "\x0D";  // CR

const std::string CMD_SET_VOLTAGE = "VP";  // Set voltage command
const std::string CMD_SET_CURRENT = "CP";  // Set current command
const std::string CMD_MON = "MON";         // Get status command
const std::string CMD_FLT = "FLT";
const std::string CMD_MNUM = "MNUM";
const std::string CMD_SNUM = "SNUM";
const std::string CMD_STARTXRAY = "ENBL1";
const std::string CMD_STOPXRAY = "ENBL0";
const std::string CMD_CLR = "CLR";
const std::string CMD_PTST = "PTST";

}  // namespace

IXS120BP120P366::IXS120BP120P366(QObject* parent)
    : QObject(parent),
      m_tcpClient(nullptr),
      m_connected(false),
      m_statusQueryTimer(nullptr),
      m_statusQueryEnabled(false)
{
    qDebug() << "Initializing IXS120BP120P366, current thread: " << QThread::currentThread();

    // 先启动工作线程
    m_thread.start();

    // 将整个 IXS120BP120P366 对象移动到工作线程
    this->moveToThread(&m_thread);
    qDebug() << "IXS120BP120P366 moved to worker thread: " << &m_thread;

    // 在工作线程中创建子对象
    QMetaObject::invokeMethod(
        this,
        [this]()
        {
            qDebug() << "Creating child objects in worker thread: " << QThread::currentThread();

            // 在工作线程中创建 TcpClient 和 Timer
            m_tcpClient = new TcpClient(this);
            m_statusQueryTimer = new QTimer(this);

            // 连接信号槽（不需要指定连接类型，因为都在同一线程）
            connect(m_tcpClient, &TcpClient::connected, this, &IXS120BP120P366::onTcpConnected);
            connect(m_tcpClient, &TcpClient::disconnected, this, &IXS120BP120P366::onTcpDisconnected);
            connect(m_tcpClient, &TcpClient::error, this, &IXS120BP120P366::onTcpError);
            connect(m_statusQueryTimer, &QTimer::timeout, this, &IXS120BP120P366::onQueryStatus);

            qDebug() << "Child objects created and connected";
        },
        Qt::BlockingQueuedConnection);

    qDebug() << "IXS120BP120P366 initialization complete";
}

IXS120BP120P366::~IXS120BP120P366()
{
    qDebug() << "Destroying IXS120BP120P366";

    // 在工作线程中清理资源
    QMetaObject::invokeMethod(
        this,
        [this]()
        {
            qDebug() << "Cleaning up in worker thread:" << QThread::currentThread();

            // 停止状态查询
            if (m_statusQueryTimer && m_statusQueryTimer->isActive())
            {
                m_statusQueryTimer->stop();
            }
            m_statusQueryEnabled = false;

            // 断开连接
            if (m_tcpClient && m_tcpClient->isConnected())
            {
                m_tcpClient->disconnectFromHost();
            }
            m_connected = false;

            qDebug() << "Cleanup complete";
        },
        Qt::BlockingQueuedConnection);

    // 停止并等待线程结束
    m_thread.quit();
    m_thread.wait();

    qDebug() << "X-ray source thread stopped";
}

IXS120BP120P366& IXS120BP120P366::Instance()
{
    static IXS120BP120P366 instance;
    return instance;
}

bool IXS120BP120P366::connectToSource(const QString& host, quint16 port)
{
    qDebug() << "connectToSource called from thread:" << QThread::currentThread();

    // 在工作线程中执行连接
    bool result = false;
    QMetaObject::invokeMethod(
        this,
        [this, host, port, &result]()
        {
            qDebug() << "connectToSource executing in thread:" << QThread::currentThread();

            if (m_connected)
            {
                qDebug() << "Already connected to X-ray source";
                result = true;
                return;
            }

            result = m_tcpClient->connectToHost(host, port);
            qDebug() << "Connect result:" << result;
        },
        Qt::BlockingQueuedConnection);

    return result;
}

void IXS120BP120P366::disconnectFromSource()
{
    qDebug() << "disconnectFromSource called from thread:" << QThread::currentThread();

    // 在工作线程中执行断开连接
    QMetaObject::invokeMethod(
        this,
        [this]()
        {
            qDebug() << "disconnectFromSource executing in thread:" << QThread::currentThread();

            if (m_tcpClient)
            {
                m_tcpClient->disconnectFromHost();
            }
            m_connected = false;
        },
        Qt::BlockingQueuedConnection);
}

bool IXS120BP120P366::isConnected() const
{
    return m_connected && m_tcpClient->isConnected();
}

bool IXS120BP120P366::setVoltage(int kV)
{
    // 将kV转换为xxxx的字符串形式，xxx.x = 030.0 – 120.0 KV
    if (kV < 30 || kV > 120)
    {
        qDebug() << "Invalid voltage value:" << kV;
        return false;
    }

    // 在工作线程中执行命令
    bool result = false;
    QMetaObject::invokeMethod(
        this,
        [this, kV, &result]()
        {
            char voltageStr[5];  // "xxxx"
            snprintf(voltageStr, sizeof(voltageStr), "%03d%1d", kV, 0);
            std::string paramStr(voltageStr);

            // 构建完整命令: STX + VP + param + CR
            std::string cmd = CMD_PREFIX + CMD_SET_VOLTAGE + paramStr + CMD_SUFFIX;
            QByteArray cmdData = QByteArray::fromStdString(cmd);
            qDebug() << "Voltage command:" << cmdData.toHex(' ') << "ASCII:" << cmdData;
            QByteArray response =
                m_tcpClient->sendDataSyncWithEndMarker(cmdData, QByteArray::fromStdString(CMD_SUFFIX), 3000);

            if (!response.isEmpty())
            {
                qDebug() << "Voltage set successfully to" << kV << "kV, response:" << response;
                result = true;
            }
            else
            {
                qDebug() << "Failed to set voltage to" << kV << "kV";
                result = false;
            }
        },
        Qt::BlockingQueuedConnection);

    return result;
}

bool IXS120BP120P366::setCurrent(int uA)
{
    // 将uA转换为xxxxx的字符串形式，x.xxxx = 0.2000 – 1.0000 mA
    if (uA < 200 || uA > 1000)
    {
        qDebug() << "Invalid current value:" << uA;
        return false;
    }

    // 在工作线程中执行命令
    bool result = false;
    QMetaObject::invokeMethod(
        this,
        [this, uA, &result]()
        {
            char str[6];  // "xxxxx"
            snprintf(str, sizeof(str), "%05d", uA * 10);
            std::string paramStr(str);

            // 构建完整命令: STX + CP + param + CR
            std::string cmd = CMD_PREFIX + CMD_SET_CURRENT + paramStr + CMD_SUFFIX;
            QByteArray cmdData = QByteArray::fromStdString(cmd);
            qDebug() << "Current command:" << cmdData.toHex(' ') << "ASCII:" << cmdData;
            QByteArray response =
                m_tcpClient->sendDataSyncWithEndMarker(cmdData, QByteArray::fromStdString(CMD_SUFFIX), 3000);

            if (!response.isEmpty())
            {
                qDebug() << "Current set successfully to" << uA << "uA, response:" << response;
                result = true;
            }
            else
            {
                qDebug() << "Failed to set current to" << uA << "uA";
                result = false;
            }
        },
        Qt::BlockingQueuedConnection);

    return result;
}

bool IXS120BP120P366::startXRay()
{
    bool result = false;
    QMetaObject::invokeMethod(
        this,
        [this, &result]()
        {
            // 构建完整命令: STX + ENBL1 + CR
            std::string cmd = CMD_PREFIX + CMD_STARTXRAY + CMD_SUFFIX;
            QByteArray cmdData = QByteArray::fromStdString(cmd);
            QByteArray response =
                m_tcpClient->sendDataSyncWithEndMarker(cmdData, QByteArray::fromStdString(CMD_SUFFIX), 3000);
            if (!response.isEmpty())
            {
                qDebug() << "X-ray started successfully, response:" << response.toHex();
                result = true;
            }
            else
            {
                qDebug() << "Failed to start X-ray";
                result = false;
            }
        },
        Qt::BlockingQueuedConnection);
    return result;
}

bool IXS120BP120P366::stopXRay()
{
    bool result = false;
    QMetaObject::invokeMethod(
        this,
        [this, &result]()
        {
            // 构建完整命令: STX + ENBL0 + CR
            std::string cmd = CMD_PREFIX + CMD_STOPXRAY + CMD_SUFFIX;
            QByteArray cmdData = QByteArray::fromStdString(cmd);
            QByteArray response =
                m_tcpClient->sendDataSyncWithEndMarker(cmdData, QByteArray::fromStdString(CMD_SUFFIX), 3000);
            if (!response.isEmpty())
            {
                qDebug() << "X-ray stopped successfully, response:" << response.toHex();
                result = true;
            }
            else
            {
                qDebug() << "Failed to stop X-ray";
                result = false;
            }
        },
        Qt::BlockingQueuedConnection);

    return result;
}

void IXS120BP120P366::clearErr()
{
    QMetaObject::invokeMethod(
        this,
        [this]()
        {
            // 构建完整命令: STX + CLR + CR
            std::string cmd = CMD_PREFIX + CMD_CLR + CMD_SUFFIX;
            QByteArray cmdData = QByteArray::fromStdString(cmd);
            QByteArray response =
                m_tcpClient->sendDataSyncWithEndMarker(cmdData, QByteArray::fromStdString(CMD_SUFFIX), 3000);
            if (!response.isEmpty())
            {
                qDebug() << "X-ray stopped successfully, response:" << response.toHex();
            }
            else
            {
                qDebug() << "Failed to stop X-ray";
            }
        },
        Qt::BlockingQueuedConnection);
}

int IXS120BP120P366::getPTST()
{
    QByteArray response;
    QMetaObject::invokeMethod(
        this,
        [this, &response]()
        {
            // 构建完整命令: STX + CLR + CR
            std::string cmd = CMD_PREFIX + CMD_PTST + CMD_SUFFIX;
            QByteArray cmdData = QByteArray::fromStdString(cmd);
            response = m_tcpClient->sendDataSyncWithEndMarker(cmdData, QByteArray::fromStdString(CMD_SUFFIX), 3000);
            if (!response.isEmpty())
            {
                qDebug() << "successfully, response:" << response.toHex();
            }
            else
            {
                qDebug() << "Failed";
            }
        },
        Qt::BlockingQueuedConnection);

    qDebug() << response;
    // 移除 STX 和 CR
    QByteArray actualData = response.mid(1, response.size() - 2);
    QString responseStr = QString::fromUtf8(actualData);
    return responseStr.toInt();
}

QByteArray IXS120BP120P366::sendDataSyncWithEndMarker(const QByteArray& data, const QByteArray& endMarker, int timeout)
{
    QByteArray response;
    QMetaObject::invokeMethod(
        this,
        [this, data, endMarker, timeout, &response]()
        {
            // 直接发送数据（调用者应该已经构建了完整命令）
            response = m_tcpClient->sendDataSyncWithEndMarker(data, endMarker, timeout);
            if (!response.isEmpty())
            {
                qDebug() << "sendDataSyncWithEndMarker response:" << response.toHex(' ');
            }
            else
            {
                qDebug() << "sendDataSyncWithEndMarker failed: no response";
            }
        },
        Qt::BlockingQueuedConnection);  // 使用阻塞调用确保同步执行

    return response;
}

void IXS120BP120P366::onTcpConnected()
{
    qDebug() << "TCP connected to X-ray source" << "Thread:" << QThread::currentThread();
    m_connected = true;
    emit connected();

    // 在工作线程中启动状态查询
    startStatusQuery(500);
}

void IXS120BP120P366::onTcpDisconnected()
{
    qDebug() << "TCP disconnected from X-ray source";
    m_connected = false;

    // Stop status query when disconnected
    stopStatusQuery();

    emit disconnected();
}

void IXS120BP120P366::onTcpError(const QString& errorMsg)
{
    qDebug() << "TCP error:" << errorMsg;
    emit error(errorMsg);
}

void IXS120BP120P366::startStatusQuery(int intervalMs)
{
    qDebug() << "Starting status query with interval:" << intervalMs << "ms";

    if (!isConnected())
    {
        qDebug() << "Cannot start status query: not connected";
        return;
    }

    QMutexLocker locker(&m_statusMutex);
    m_statusQueryEnabled = true;

    // Start timer in the worker thread using QMetaObject::invokeMethod
    QMetaObject::invokeMethod(
        m_statusQueryTimer, [this, intervalMs]() { m_statusQueryTimer->start(intervalMs); }, Qt::QueuedConnection);

    qDebug() << "Status query started";
}

void IXS120BP120P366::stopStatusQuery()
{
    qDebug() << "Stopping status query";

    QMutexLocker locker(&m_statusMutex);
    m_statusQueryEnabled = false;

    // Stop timer in the worker thread
    QMetaObject::invokeMethod(m_statusQueryTimer, [this]() { m_statusQueryTimer->stop(); }, Qt::QueuedConnection);

    qDebug() << "Status query stopped";
}

bool IXS120BP120P366::isStatusQueryRunning() const
{
    QMutexLocker locker(&m_statusMutex);
    return m_statusQueryEnabled;
}

XRaySourceStatus IXS120BP120P366::getCurrentStatus() const
{
    QMutexLocker locker(&m_statusMutex);
    return m_currentStatus;
}

void IXS120BP120P366::onQueryStatus()
{
    if (!m_statusQueryEnabled || !isConnected())
    {
        return;
    }

    // 构建完整命令: STX + MON + CR
    std::string cmd = CMD_PREFIX + CMD_MON + CMD_SUFFIX;
    QByteArray cmdData = QByteArray::fromStdString(cmd);
    QByteArray response = m_tcpClient->sendDataSyncWithEndMarker(cmdData, QByteArray::fromStdString(CMD_SUFFIX), 1000);

    if (!response.isEmpty())
    {
        // 验证响应格式并提取数据
        if (response.size() >= 2 && response.at(0) == 0x02 && response.at(response.size() - 1) == 0x0D)
        {
            // 移除 STX 和 CR
            QByteArray actualData = response.mid(1, response.size() - 2);
            QString responseStr = QString::fromUtf8(actualData);
            parseMONResponse(responseStr);
        }
        else
        {
            qDebug() << "Invalid status response format:" << response.toHex();
        }
    }

    cmd = CMD_PREFIX + CMD_FLT + CMD_SUFFIX;
    cmdData = QByteArray::fromStdString(cmd);
    response = m_tcpClient->sendDataSyncWithEndMarker(cmdData, QByteArray::fromStdString(CMD_SUFFIX), 1000);

    if (!response.isEmpty())
    {
        // 处理 FLT 响应（类似处理 MON 响应）
        if (response.size() >= 2 && response.at(0) == 0x02 && response.at(response.size() - 1) == 0x0D)
        {
            // 移除 STX 和 CR
            QByteArray actualData = response.mid(1, response.size() - 2);
            QString responseStr = QString::fromUtf8(actualData);
            parseFTLResponse(responseStr);
        }
        else
        {
            qDebug() << "Invalid status response format:" << response.toHex();
        }
    }

    emit statusUpdated(m_currentStatus);
}

void IXS120BP120P366::parseMONResponse(const QString& response)
{
    // response的格式：vvvv<SP>ccccc<SP>tttt<SP>ffff <SP> bbbb
    // vvv.v = 000.0 – [max] kV
    // c.cccc = 0.0000 – [max] mA
    // ttt.t = 000.0 – [070.0] °C
    // f.fff = 0.000 – 9.999 A
    // bb.bb = 00.00 – [29.99] VDC
    QStringList parts = response.split(' ', Qt::SkipEmptyParts);
    if (parts.size() < 5)
    {
        qDebug() << "Incomplete MON response:" << response;
        return;
    }

    bool ok;
    // 解析电压 (kV) - vvvv 格式，除以10得到 vvv.v
    int voltageRaw = parts[0].toInt(&ok);
    if (ok)
    {
        m_currentStatus.voltage = voltageRaw / 10.0;
    }
    else
    {
        qDebug() << "Failed to parse voltage:" << parts[0];
        m_currentStatus.voltage = 0.0;
    }

    // 解析电流 (mA) - ccccc 格式，除以10000得到 c.cccc
    int currentRaw = parts[1].toInt(&ok);
    if (ok)
    {
        m_currentStatus.current = currentRaw / 10000.0;
    }
    else
    {
        qDebug() << "Failed to parse current:" << parts[1];
        m_currentStatus.current = 0.0;
    }

    // 解析温度 (°C) - tttt 格式，除以10得到 ttt.t (仅用于日志)
    int tempRaw = parts[2].toInt(&ok);
    if (ok)
    {
        m_currentStatus.temperature = tempRaw / 10.0;
    }

    // 解析灯丝电流 (A) - ffff 格式，除以1000得到 f.fff (仅用于日志)
    int filamentRaw = parts[3].toInt(&ok);
    if (ok)
    {
        m_currentStatus.filamentCurrent = filamentRaw / 1000.0;
    }

    // 解析电压 (VDC) - bbbb 格式，除以100得到 bb.bb (仅用于日志)
    double vdc = 0.0;
    int vdcRaw = parts[4].toInt(&ok);
    if (ok)
    {
        vdc = vdcRaw / 100.0;
    }

    // 判断X射线是否开启（根据电流是否大于0）
    m_currentStatus.xrayOn = (m_currentStatus.current > 0.001);  // 使用小阈值避免浮点误差
    // qDebug() << "Status updated - X-ray:" << m_currentStatus.xrayOn << "Voltage:" << m_currentStatus.voltage << "kV"
    //          << "Current:" << m_currentStatus.current << "uA"
    //          << "Temperature:" << m_currentStatus.temperature << "°C"
    //          << "Filament:" << m_currentStatus.filamentCurrent << "u"
    //          << "VDC:" << vdc << "V"
    //          << "Fault:" << m_currentStatus.faultStatus << "Warmup:" << m_currentStatus.warmupComplete
    //          << "Interlock:" << m_currentStatus.interlock;
}

void IXS120BP120P366::parseFTLResponse(const QString& response)
{
    // FLT 响应格式: x<SP>x<SP>x<SP>x<SP>x<SP>x<SP>x<SP>x<SP>x<SP>x<SP>x
    // 11个字段，每个字段表示不同的故障/状态位
    // 0 = 正常, 1 = 故障/激活

    QStringList parts = response.split(' ', Qt::SkipEmptyParts);
    if (parts.size() < 11)
    {
        qDebug() << "Incomplete FLT response:" << response;
        return;
    }

    bool ok;
    QVector<int> faultBits;
    bool anyFault = false;

    // 解析所有11个故障位
    for (int i = 0; i < 11; ++i)
    {
        int bit = parts[i].toInt(&ok);
        if (!ok)
        {
            qDebug() << "Failed to parse fault bit" << i << ":" << parts[i];
            bit = 0;
        }
        faultBits.append(bit);
        if (bit != 0)
        {
            anyFault = true;
        }
    }

    // 更新故障状态和相关字段
    m_currentStatus.faultStatus = anyFault;

    // 根据具体的故障位更新其他状态字段
    m_currentStatus.interlock = faultBits[8];

    // 输出详细的故障信息用于调试
    if (anyFault)
    {
        QString faultInfo = "Fault detected - Bits:";
        for (int i = 0; i < faultBits.size(); ++i)
        {
            if (faultBits[i] != 0)
            {
                faultInfo += QString(" [%1]=%2").arg(i).arg(faultBits[i]);
                emit error(QString("Fault bit %1 active (value=%2)").arg(i).arg(faultBits[i]));
            }
        }
        qDebug() << faultInfo;
    }
}

int IXS120BP120P366::parsePTSTResponse(const QString& response)
{
    return 0;
}
