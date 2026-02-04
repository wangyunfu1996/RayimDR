#include "IXS120BP120P366.h"
#include <QDebug>
#include <QMetaObject>

#include "TcpClient.h"
#include "../Components/XGlobal.h"

#pragma warning(disable : 4996)  // disable deprecated function warning

// ============================================================================
// Protocol Constants
// ============================================================================

namespace
{
// Communication protocol
const int CMD_MAX_LENGTH = 48;
const std::string CMD_PREFIX = "\x02";  // STX (Start of Text)
const std::string CMD_SUFFIX = "\x0D";  // CR (Carriage Return)

// Command strings
const std::string CMD_SET_VOLTAGE = "VP";   // Set voltage
const std::string CMD_SET_CURRENT = "CP";   // Set current
const std::string CMD_MON = "MON";          // Monitor status
const std::string CMD_FLT = "FLT";          // Fault status
const std::string CMD_PTST = "PTST";        // Prewarning time
const std::string CMD_MNUM = "MNUM";        // Model number
const std::string CMD_SNUM = "SNUM";        // Serial number
const std::string CMD_STARTXRAY = "ENBL1";  // Enable X-ray
const std::string CMD_STOPXRAY = "ENBL0";   // Disable X-ray
const std::string CMD_CLR = "CLR";          // Clear error
const std::string CMD_STAT = "STAT";        // X-ray is on or off

// Protocol parameters
constexpr int DEFAULT_TIMEOUT = 3000;       // milliseconds
constexpr int STATUS_QUERY_TIMEOUT = 1000;  // milliseconds
}  // namespace

// ============================================================================
// Constructor / Destructor
// ============================================================================

IXS120BP120P366::IXS120BP120P366(QObject* parent)
    : QObject(parent),
      m_tcpClient(nullptr),
      m_connected(false),
      m_statusQueryTimer(nullptr),
      m_statusQueryEnabled(false)
{
    qDebug() << "[IXS120BP120P366] Initializing - Main thread:" << QThread::currentThread();

    // Start worker thread
    m_thread.start();

    // Move this object to worker thread
    moveToThread(&m_thread);
    qDebug() << "[IXS120BP120P366] Moved to worker thread:" << &m_thread;

    // Create child objects in worker thread
    QMetaObject::invokeMethod(
        this,
        [this]()
        {
            qDebug() << "[IXS120BP120P366] Creating child objects in worker thread:" << QThread::currentThread();

            // Create TCP client and status query timer
            m_tcpClient = new TcpClient();
            m_statusQueryTimer = new QTimer();

            // Connect signals (same thread, no connection type needed)
            connect(m_tcpClient, &TcpClient::connected, this, &IXS120BP120P366::onTcpConnected);
            connect(m_tcpClient, &TcpClient::disconnected, this, &IXS120BP120P366::onTcpDisconnected);
            connect(m_tcpClient, &TcpClient::error, this, &IXS120BP120P366::onTcpError);
            connect(m_statusQueryTimer, &QTimer::timeout, this, &IXS120BP120P366::onQueryStatus);

            qDebug() << "[IXS120BP120P366] Initialization complete";
        },
        Qt::BlockingQueuedConnection);
}

IXS120BP120P366::~IXS120BP120P366()
{
    qDebug() << "[IXS120BP120P366] Destroying X-ray source controller";

    // Clean up resources in worker thread
    QMetaObject::invokeMethod(
        this,
        [this]()
        {
            qDebug() << "[IXS120BP120P366] Cleanup in worker thread:" << QThread::currentThread();

            // Stop status query
            if (m_statusQueryTimer && m_statusQueryTimer->isActive())
            {
                m_statusQueryTimer->stop();
            }
            m_statusQueryEnabled = false;

            // Disconnect from host
            if (m_tcpClient && m_tcpClient->isConnected())
            {
                m_tcpClient->disconnectFromHost();
            }
            m_connected = false;

            qDebug() << "[IXS120BP120P366] Cleanup complete";
        },
        Qt::BlockingQueuedConnection);

    // Stop and wait for thread to finish
    m_thread.quit();
    m_thread.wait();

    qDebug() << "[IXS120BP120P366] Worker thread stopped";
}

// ============================================================================
// Public Interface Methods
// ============================================================================

IXS120BP120P366& IXS120BP120P366::Instance()
{
    static IXS120BP120P366 instance;
    return instance;
}

bool IXS120BP120P366::connectToSource(const QString& host, quint16 port)
{
    qDebug() << "[Connect] Request from thread:" << QThread::currentThread();

    bool result = false;
    QMetaObject::invokeMethod(
        this,
        [this, host, port, &result]()
        {
            qDebug() << "[Connect] Executing in worker thread:" << QThread::currentThread();

            if (m_connected)
            {
                qDebug() << "[Connect] Already connected to X-ray source";
                result = true;
                return;
            }

            qDebug() << "[Connect] Connecting to" << host << ":" << port;
            result = m_tcpClient->connectToHost(host, port);
            qDebug() << "[Connect] Result:" << (result ? "Success" : "Failed");
        },
        Qt::BlockingQueuedConnection);

    return result;
}

void IXS120BP120P366::disconnectFromSource()
{
    qDebug() << "[Disconnect] Request from thread:" << QThread::currentThread();

    QMetaObject::invokeMethod(
        this,
        [this]()
        {
            qDebug() << "[Disconnect] Executing in worker thread:" << QThread::currentThread();

            if (m_tcpClient)
            {
                m_tcpClient->disconnectFromHost();
            }
            m_connected = false;

            qDebug() << "[Disconnect] Disconnected from X-ray source";
        },
        Qt::BlockingQueuedConnection);
}

bool IXS120BP120P366::isConnected() const
{
    return m_connected && m_tcpClient && m_tcpClient->isConnected();
}

bool IXS120BP120P366::setVoltage(int kV)
{
    // Validate voltage range: 30.0 - 120.0 kV
    if (kV < XRAY_MIN_VOLTAGE || kV > XRAY_MAX_VOLTAGE)
    {
        qDebug() << "[SetVoltage] Invalid voltage:" << kV << "kV (valid range:" << XRAY_MIN_VOLTAGE << "-"
                 << XRAY_MAX_VOLTAGE << "kV)";
        return false;
    }

    bool result = false;
    QMetaObject::invokeMethod(
        this,
        [this, kV, &result]()
        {
            // Format: "xxxx" representing xxx.x kV (e.g., "0300" = 30.0 kV)
            char voltageStr[5];
            snprintf(voltageStr, sizeof(voltageStr), "%03d%1d", kV, 0);
            std::string paramStr(voltageStr);

            // Build command: STX + VP + parameter + CR
            std::string cmd = CMD_PREFIX + CMD_SET_VOLTAGE + paramStr + CMD_SUFFIX;
            QByteArray cmdData = QByteArray::fromStdString(cmd);

            qDebug() << "[SetVoltage] Setting to" << kV << "kV - Command:" << cmdData;

            QByteArray response =
                m_tcpClient->sendDataSyncWithEndMarker(cmdData, QByteArray::fromStdString(CMD_SUFFIX), DEFAULT_TIMEOUT);

            if (!response.isEmpty())
            {
                qDebug() << "[SetVoltage] Success - Voltage set to" << kV << "kV";
                result = true;
            }
            else
            {
                qDebug() << "[SetVoltage] Failed - No response from X-ray source";
                result = false;
            }
        },
        Qt::BlockingQueuedConnection);

    return result;
}

bool IXS120BP120P366::setCurrent(int uA)
{
    // Validate current range: 0.2000 - 1.0000 mA (200 - 1000 uA)
    if (uA < XRAY_MIN_CURRENT || uA > XRAY_MAX_CURRENT)
    {
        qDebug() << "[SetCurrent] Invalid current:" << uA << "uA (valid range:" << XRAY_MIN_CURRENT << "-"
                 << XRAY_MAX_CURRENT << "uA)";
        return false;
    }

    bool result = false;
    QMetaObject::invokeMethod(
        this,
        [this, uA, &result]()
        {
            // Format: "xxxxx" representing x.xxxx mA (e.g., "02000" = 0.2000 mA = 200 uA)
            char currentStr[6];
            snprintf(currentStr, sizeof(currentStr), "%05d", uA * 10);
            std::string paramStr(currentStr);

            // Build command: STX + CP + parameter + CR
            std::string cmd = CMD_PREFIX + CMD_SET_CURRENT + paramStr + CMD_SUFFIX;
            QByteArray cmdData = QByteArray::fromStdString(cmd);

            qDebug() << "[SetCurrent] Setting to" << uA << "uA - Command:" << cmdData;

            QByteArray response =
                m_tcpClient->sendDataSyncWithEndMarker(cmdData, QByteArray::fromStdString(CMD_SUFFIX), DEFAULT_TIMEOUT);

            if (!response.isEmpty())
            {
                qDebug() << "[SetCurrent] Success - Current set to" << uA << "uA";
                result = true;
            }
            else
            {
                qDebug() << "[SetCurrent] Failed - No response from X-ray source";
                result = false;
            }
        },
        Qt::BlockingQueuedConnection);

    return result;
}

bool IXS120BP120P366::startXRay()
{
    qDebug() << "[StartXRay] Enabling X-ray emission";

    bool result = false;
    QMetaObject::invokeMethod(
        this,
        [this, &result]()
        {
            // Build command: STX + ENBL1 + CR
            std::string cmd = CMD_PREFIX + CMD_STARTXRAY + CMD_SUFFIX;
            QByteArray cmdData = QByteArray::fromStdString(cmd);

            QByteArray response =
                m_tcpClient->sendDataSyncWithEndMarker(cmdData, QByteArray::fromStdString(CMD_SUFFIX), DEFAULT_TIMEOUT);

            if (!response.isEmpty())
            {
                qDebug() << "[StartXRay] Success - X-ray emission enabled";
                result = true;
            }
            else
            {
                qDebug() << "[StartXRay] Failed - No response from X-ray source";
                result = false;
            }
        },
        Qt::BlockingQueuedConnection);

    return result;
}

bool IXS120BP120P366::stopXRay()
{
    qDebug() << "[StopXRay] Disabling X-ray emission";

    bool result = false;
    QMetaObject::invokeMethod(
        this,
        [this, &result]()
        {
            // Build command: STX + ENBL0 + CR
            std::string cmd = CMD_PREFIX + CMD_STOPXRAY + CMD_SUFFIX;
            QByteArray cmdData = QByteArray::fromStdString(cmd);

            QByteArray response =
                m_tcpClient->sendDataSyncWithEndMarker(cmdData, QByteArray::fromStdString(CMD_SUFFIX), DEFAULT_TIMEOUT);

            if (!response.isEmpty())
            {
                qDebug() << "[StopXRay] Success - X-ray emission disabled";
                result = true;
            }
            else
            {
                qDebug() << "[StopXRay] Failed - No response from X-ray source";
                result = false;
            }
        },
        Qt::BlockingQueuedConnection);

    return result;
}

void IXS120BP120P366::clearErr()
{
    qDebug() << "[ClearError] Clearing X-ray source error state";

    QMetaObject::invokeMethod(
        this,
        [this]()
        {
            // Build command: STX + CLR + CR
            std::string cmd = CMD_PREFIX + CMD_CLR + CMD_SUFFIX;
            QByteArray cmdData = QByteArray::fromStdString(cmd);

            QByteArray response =
                m_tcpClient->sendDataSyncWithEndMarker(cmdData, QByteArray::fromStdString(CMD_SUFFIX), DEFAULT_TIMEOUT);

            if (!response.isEmpty())
            {
                qDebug() << "[ClearError] Success - Error state cleared";
            }
            else
            {
                qDebug() << "[ClearError] Failed - No response from X-ray source";
            }
        },
        Qt::BlockingQueuedConnection);
}

bool IXS120BP120P366::xRayIsOn()
{
    qDebug() << "Querying STAT";

    QByteArray response;
    QMetaObject::invokeMethod(
        this,
        [this, &response]()
        {
            // Build command: STX + STAT + CR
            std::string cmd = CMD_PREFIX + CMD_STAT + CMD_SUFFIX;
            QByteArray cmdData = QByteArray::fromStdString(cmd);

            response =
                m_tcpClient->sendDataSyncWithEndMarker(cmdData, QByteArray::fromStdString(CMD_SUFFIX), DEFAULT_TIMEOUT);

            if (!response.isEmpty())
            {
                qDebug() << "Received response:" << response;
            }
            else
            {
                qDebug() << "Failed - No response from X-ray source";
            }
        },
        Qt::BlockingQueuedConnection);

    if (response.size() < 3)
    {
        qDebug() << "Invalid response format";
        return 0;
    }

    // Remove STX and CR
    QByteArray actualData = response.mid(1, response.size() - 2);
    QString responseStr = QString::fromUtf8(actualData);
    int on = responseStr.toInt();
    return on == 1;
}

int IXS120BP120P366::getPTST()
{
    qDebug() << "[GetPTST] Querying pre-warmup time";

    QByteArray response;
    QMetaObject::invokeMethod(
        this,
        [this, &response]()
        {
            // Build command: STX + PTST + CR
            std::string cmd = CMD_PREFIX + CMD_PTST + CMD_SUFFIX;
            QByteArray cmdData = QByteArray::fromStdString(cmd);

            response =
                m_tcpClient->sendDataSyncWithEndMarker(cmdData, QByteArray::fromStdString(CMD_SUFFIX), DEFAULT_TIMEOUT);

            if (!response.isEmpty())
            {
                qDebug() << "[GetPTST] Received response:" << response;
            }
            else
            {
                qDebug() << "[GetPTST] Failed - No response from X-ray source";
            }
        },
        Qt::BlockingQueuedConnection);

    if (response.size() < 3)
    {
        qDebug() << "[GetPTST] Invalid response format";
        return 0;
    }

    // Remove STX and CR
    QByteArray actualData = response.mid(1, response.size() - 2);
    QString responseStr = QString::fromUtf8(actualData);
    int ptst = responseStr.toInt();

    qDebug() << "[GetPTST] Pre-warmup time:" << ptst << "seconds";
    return ptst;
}

QByteArray IXS120BP120P366::sendDataSyncWithEndMarker(const QByteArray& data, const QByteArray& endMarker, int timeout)
{
    QByteArray response;
    QMetaObject::invokeMethod(
        this,
        [this, data, endMarker, timeout, &response]()
        {
            // Forward data to TCP client (caller should have built complete command)
            response = m_tcpClient->sendDataSyncWithEndMarker(data, endMarker, timeout);

            if (!response.isEmpty())
            {
                qDebug() << "[SendData] Received response:" << response;
            }
            else
            {
                qDebug() << "[SendData] No response received";
            }
        },
        Qt::BlockingQueuedConnection);

    return response;
}

// ============================================================================
// Event Handlers
// ============================================================================

void IXS120BP120P366::onTcpConnected()
{
    qDebug() << "[Event] TCP connected to X-ray source - Thread:" << QThread::currentThread();
    m_connected = true;
    emit connected();

    // Start status query in worker thread
    startStatusQuery(500);
}

void IXS120BP120P366::onTcpDisconnected()
{
    qDebug() << "[Event] TCP disconnected from X-ray source";
    m_connected = false;

    // Stop status query when disconnected
    stopStatusQuery();

    emit disconnected();
}

void IXS120BP120P366::onTcpError(const QString& errorMsg)
{
    qDebug() << "[Event] TCP error:" << errorMsg;
    emit xrayError(errorMsg);
}

// ============================================================================
// Status Query Control
// ============================================================================

void IXS120BP120P366::startStatusQuery(int intervalMs)
{
    qDebug() << "[StatusQuery] Starting with interval:" << intervalMs << "ms";

    if (!isConnected())
    {
        qDebug() << "[StatusQuery] Cannot start: not connected";
        return;
    }

    QMutexLocker locker(&m_statusMutex);
    m_statusQueryEnabled = true;

    // Start timer in worker thread
    QMetaObject::invokeMethod(
        m_statusQueryTimer, [this, intervalMs]() { m_statusQueryTimer->start(intervalMs); }, Qt::QueuedConnection);

    qDebug() << "[StatusQuery] Started successfully";
}

void IXS120BP120P366::stopStatusQuery()
{
    qDebug() << "[StatusQuery] Stopping";

    QMutexLocker locker(&m_statusMutex);
    m_statusQueryEnabled = false;

    // Stop timer in worker thread
    QMetaObject::invokeMethod(m_statusQueryTimer, [this]() { m_statusQueryTimer->stop(); }, Qt::QueuedConnection);

    qDebug() << "[StatusQuery] Stopped";
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

    // Query MON (Monitor status)
    std::string cmd = CMD_PREFIX + CMD_MON + CMD_SUFFIX;
    QByteArray cmdData = QByteArray::fromStdString(cmd);
    QByteArray response =
        m_tcpClient->sendDataSyncWithEndMarker(cmdData, QByteArray::fromStdString(CMD_SUFFIX), STATUS_QUERY_TIMEOUT);

    if (!response.isEmpty())
    {
        // Validate response format (STX...CR)
        if (response.size() >= 2 && response.at(0) == 0x02 && response.at(response.size() - 1) == 0x0D)
        {
            // Remove STX and CR
            QByteArray actualData = response.mid(1, response.size() - 2);
            QString responseStr = QString::fromUtf8(actualData);
            parseMONResponse(responseStr);
        }
        else
        {
            qDebug() << "[StatusQuery] Invalid MON response format:" << response;
        }
    }

    // Query FLT (Fault status)
    cmd = CMD_PREFIX + CMD_FLT + CMD_SUFFIX;
    cmdData = QByteArray::fromStdString(cmd);
    response =
        m_tcpClient->sendDataSyncWithEndMarker(cmdData, QByteArray::fromStdString(CMD_SUFFIX), STATUS_QUERY_TIMEOUT);

    if (!response.isEmpty())
    {
        // Validate response format (STX...CR)
        if (response.size() >= 2 && response.at(0) == 0x02 && response.at(response.size() - 1) == 0x0D)
        {
            // Remove STX and CR
            QByteArray actualData = response.mid(1, response.size() - 2);
            QString responseStr = QString::fromUtf8(actualData);
            parseFTLResponse(responseStr);
        }
        else
        {
            qDebug() << "[StatusQuery] Invalid FLT response format:" << response;
        }
    }

    emit statusUpdated(m_currentStatus);
}

// ============================================================================
// Response Parsing Methods
// ============================================================================

void IXS120BP120P366::parseMONResponse(const QString& response)
{
    // Response format: vvvv<SP>ccccc<SP>tttt<SP>ffff<SP>bbbb
    // vvv.v = 000.0 – [max] kV (voltage)
    // c.cccc = 0.0000 – [max] mA (current)
    // ttt.t = 000.0 – 070.0 °C (temperature)
    // f.fff = 0.000 – 9.999 A (filament current)
    // bb.bb = 00.00 – 29.99 VDC (voltage DC)

    QStringList parts = response.split(' ', Qt::SkipEmptyParts);
    if (parts.size() < 5)
    {
        qDebug() << "[ParseMON] Incomplete response:" << response;
        return;
    }

    bool ok;

    // Parse voltage (kV) - format: vvvv, divide by 10 to get vvv.v
    int voltageRaw = parts[0].toInt(&ok);
    if (ok)
    {
        m_currentStatus.voltage = voltageRaw / 10.0;
    }
    else
    {
        qDebug() << "[ParseMON] Failed to parse voltage:" << parts[0];
        m_currentStatus.voltage = 0.0;
    }

    // Parse current (mA) - format: ccccc, divide by 10000 to get c.cccc
    int currentRaw = parts[1].toInt(&ok);
    if (ok)
    {
        m_currentStatus.current = currentRaw / 10000.0;
    }
    else
    {
        qDebug() << "[ParseMON] Failed to parse current:" << parts[1];
        m_currentStatus.current = 0.0;
    }

    // Parse temperature (°C) - format: tttt, divide by 10 to get ttt.t
    int tempRaw = parts[2].toInt(&ok);
    if (ok)
    {
        m_currentStatus.temperature = tempRaw / 10.0;
    }

    // Parse filament current (A) - format: ffff, divide by 1000 to get f.fff
    int filamentRaw = parts[3].toInt(&ok);
    if (ok)
    {
        m_currentStatus.filamentCurrent = filamentRaw / 1000.0;
    }

    // Parse bias voltage (VDC) - format: bbbb, divide by 100 to get bb.bb
    double vdc = 0.0;
    int vdcRaw = parts[4].toInt(&ok);
    if (ok)
    {
        vdc = vdcRaw / 100.0;
    }
}

void IXS120BP120P366::parseFTLResponse(const QString& response)
{
    // FLT response format: x<SP>x<SP>x<SP>x<SP>x<SP>x<SP>x<SP>x<SP>x<SP>x<SP>x
    // 11 fields, each representing different fault/status bits
    // 0 = Normal, 1 = Fault/Active
    // Note: Specific bit mappings should be verified against device documentation

    QStringList parts = response.split(' ', Qt::SkipEmptyParts);
    if (parts.size() < 11)
    {
        qDebug() << "[ParseFLT] Incomplete response:" << response;
        return;
    }

    bool ok;
    bool hasError = false;
    bool stateChanged = false;

    // 解析当前的故障位
    std::vector<int> currentFaultBits(11, 0);
    for (int i = 0; i < 11; ++i)
    {
        int bit = parts[i].toInt(&ok);
        if (!ok)
        {
            qDebug() << "[ParseFLT] Failed to parse bit" << i << ":" << parts[i];
            bit = 0;
        }
        currentFaultBits[i] = bit;
        if (bit != 0)
        {
            hasError = true;
        }
    }

    // 比较当前状态与上一次状态是否一致
    if (m_currentStatus.faultBits.size() == currentFaultBits.size())
    {
        for (size_t i = 0; i < currentFaultBits.size(); ++i)
        {
            if (m_currentStatus.faultBits[i] != currentFaultBits[i])
            {
                stateChanged = true;
                break;
            }
        }
    }
    else
    {
        // 首次初始化或大小不匹配，视为状态变化
        stateChanged = true;
    }

    // 只有在状态发生变化且存在错误时才触发 anyFault
    bool anyFault = stateChanged && hasError;

    // 更新当前状态
    m_currentStatus.faultBits = currentFaultBits;

    // Update specific status fields based on fault bits
    // Note: Bit 8 is assumed to be interlock status (verify with device manual)
    if (currentFaultBits.size() > 8)
    {
        m_currentStatus.interlock = currentFaultBits[8];
    }

    // Log detailed fault information for debugging (only when fault state changes and has errors)
    if (anyFault)
    {
        QString faultInfo = "[ParseFLT] Fault state changed - New faults detected:";
        for (size_t i = 0; i < currentFaultBits.size(); ++i)
        {
            if (currentFaultBits[i] != 0)
            {
                faultInfo += QString(" Bit[%1]=%2").arg(i).arg(currentFaultBits[i]);
            }
        }
        qDebug() << faultInfo;
        emit xrayError(faultInfo);
    }
    else if (stateChanged && !hasError)
    {
        // 状态变化但无错误（故障已清除）
        qDebug() << "[ParseFLT] Fault state changed - All faults cleared";
        emit xrayErrorCleared();
    }
}
