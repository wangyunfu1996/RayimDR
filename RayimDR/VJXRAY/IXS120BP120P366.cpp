#include "IXS120BP120P366.h"
#include "TcpClient.h"
#include <QDebug>

#pragma warning(disable : 4996)  // disable deprecated function warning

namespace
{
const int CMD_MAX_LENGTH = 48;
const std::string CMD_PREFIX = "\x02";  // STX
const std::string CMD_SUFFIX = "\x0D";  // CR

const std::string CMD_SET_VOLTAGE = "VP";  // Set voltage command
const std::string CMD_SET_CURRENT = "CP";  // Set current command
}  // namespace

IXS120BP120P366::IXS120BP120P366(QObject* parent)
    : QObject(parent), m_tcpClient(nullptr), m_connected(false), m_responseReceived(false)
{
    qDebug() << "Initializing IXS120BP120P366";

    m_tcpClient = new TcpClient();
    m_tcpClient->moveToThread(&m_thread);

    connect(m_tcpClient, &TcpClient::connected, this, &IXS120BP120P366::onTcpConnected, Qt::QueuedConnection);
    connect(m_tcpClient, &TcpClient::disconnected, this, &IXS120BP120P366::onTcpDisconnected, Qt::QueuedConnection);
    connect(m_tcpClient, &TcpClient::error, this, &IXS120BP120P366::onTcpError, Qt::QueuedConnection);
    connect(m_tcpClient, &TcpClient::dataReceived, this, &IXS120BP120P366::onTcpDataReceived, Qt::QueuedConnection);

    m_thread.start();
    qDebug() << "X-ray source thread started";
}

IXS120BP120P366::~IXS120BP120P366()
{
    qDebug() << "Destroying IXS120BP120P366";
    disconnectFromSource();

    m_thread.quit();
    m_thread.wait();

    if (m_tcpClient)
    {
        delete m_tcpClient;
        m_tcpClient = nullptr;
    }

    qDebug() << "X-ray source thread stopped";
}

IXS120BP120P366& IXS120BP120P366::Instance()
{
    static IXS120BP120P366 instance;
    return instance;
}

bool IXS120BP120P366::connectToSource(const QString& host, quint16 port)
{
    qDebug() << "Connecting to X-ray source at" << host << ":" << port;

    if (m_connected)
    {
        qDebug() << "Already connected to X-ray source";
        return true;
    }

    return m_tcpClient->connectToHost(host, port);
}

void IXS120BP120P366::disconnectFromSource()
{
    qDebug() << "Disconnecting from X-ray source";
    m_tcpClient->disconnectFromHost();
    m_connected = false;
}

bool IXS120BP120P366::isConnected() const
{
    return m_connected && m_tcpClient->isConnected();
}

bool IXS120BP120P366::sendCommand(const std::string& cmd, const std::string& param)
{
    if (!isConnected())
    {
        qDebug() << "Cannot do command: not connected";
        return false;
    }

    try
    {
        // Build command string: STX + cmd + param + CR
        std::string fullCmd = cmd + param;
        size_t totalSize = fullCmd.size() + 2;  // +2 for STX and CR

        if (totalSize > 50)
        {
            qDebug() << "Command too long:" << totalSize << "bytes";
            return false;
        }

        char buffer[50];
        buffer[0] = 0x02;  // STX
        memcpy(&buffer[1], fullCmd.c_str(), fullCmd.size());
        buffer[1 + fullCmd.size()] = 0x0D;  // CR

        qDebug() << "Sending command to X-ray source:" << QString::fromStdString(cmd)
                 << (param.empty() ? "" : (" with param: " + QString::fromStdString(param)));

        m_tcpClient->sendData(QByteArray(buffer, static_cast<int>(totalSize)));
    }
    catch (const std::exception& ex)
    {
        qDebug() << "Error sending command to X-ray source:" << ex.what();
        return false;
    }

    return true;
}

bool IXS120BP120P366::sendCmdAndWaitForResponse(const std::string& cmd, const std::string& param, QString& response,
                                                int timeoutMs)
{
    if (!isConnected())
    {
        qDebug() << "Cannot send command: not connected";
        return false;
    }

    // 设置期望的响应前缀（命令的前两个字符）
    QMutexLocker locker(&m_responseMutex);
    m_responseReceived = false;
    m_lastResponse.clear();
    m_expectedResponsePrefix = QString::fromStdString(cmd);

    // 发送命令
    // 注意：sendCommand 内部调用 m_tcpClient->sendData()
    // 由于 TcpClient 在另一个线程中，我们需要确保线程安全
    // Qt 的信号槽机制会自动处理跨线程调用
    if (!sendCommand(cmd, param))
    {
        m_expectedResponsePrefix.clear();
        qDebug() << "Failed to send command:" << QString::fromStdString(cmd);
        return false;
    }

    qDebug() << "Command sent:" << QString::fromStdString(cmd + param) << ", waiting for response...";

    // 阻塞等待响应
    bool success = m_responseCondition.wait(&m_responseMutex, timeoutMs);

    if (!success)
    {
        qDebug() << "Timeout waiting for response to command:" << QString::fromStdString(cmd);
        m_expectedResponsePrefix.clear();
        return false;
    }

    if (!m_responseReceived)
    {
        qDebug() << "No response received for command:" << QString::fromStdString(cmd);
        m_expectedResponsePrefix.clear();
        return false;
    }

    // 返回响应
    response = QString::fromUtf8(m_lastResponse);
    qDebug() << "Command executed successfully, response:" << response;
    m_expectedResponsePrefix.clear();
    return true;
}

bool IXS120BP120P366::setVoltage(int kV)
{
    // 将kV转换为xxxx的字符串形式，xxx.x = 030.0 – 120.0 KV
    if (kV < 30 || kV > 120)
    {
        qDebug() << "Invalid voltage value:" << kV;
        return false;
    }

    char voltageStr[5];  // "xxxx"
    snprintf(voltageStr, sizeof(voltageStr), "%03d%1d", kV, 0);
    std::string paramStr(voltageStr);

    QString response;
    if (sendCmdAndWaitForResponse(CMD_SET_VOLTAGE, paramStr, response, 3000))
    {
        qDebug() << "Voltage set successfully to" << kV << "kV, response:" << response;
        return true;
    }

    qDebug() << "Failed to set voltage to" << kV << "kV";
    return false;
}

bool IXS120BP120P366::setCurrent(int uA)
{
    // 将uA转换为xxxxx的字符串形式，x.xxxx = 0.2000 – 1.0000 mA
    if (uA < 200 || uA > 1000)
    {
        qDebug() << "Invalid current value:" << uA;
        return false;
    }

    char currentStr[6];  // "xxxxx"
    snprintf(currentStr, sizeof(currentStr), "%03d%1d", uA / 1000, (uA % 1000) / 100);
    std::string paramStr(currentStr);

    QString response;
    if (sendCmdAndWaitForResponse(CMD_SET_CURRENT, paramStr, response, 3000))
    {
        qDebug() << "Current set successfully to" << uA << "uA, response:" << response;
        return true;
    }

    qDebug() << "Failed to set current to" << uA << "uA";
    return false;
}

void IXS120BP120P366::onTcpConnected()
{
    qDebug() << "TCP connected to X-ray source";
    m_connected = true;
    emit connected();
}

void IXS120BP120P366::onTcpDisconnected()
{
    qDebug() << "TCP disconnected from X-ray source";
    m_connected = false;

    // 唤醒所有等待响应的线程，避免它们永久阻塞
    QMutexLocker locker(&m_responseMutex);
    m_expectedResponsePrefix.clear();
    m_responseReceived = false;
    m_responseCondition.wakeAll();
    locker.unlock();

    emit disconnected();
}

void IXS120BP120P366::onTcpError(const QString& errorMsg)
{
    qDebug() << "TCP error:" << errorMsg;
    emit error(errorMsg);
}

void IXS120BP120P366::onTcpDataReceived(const QByteArray& data)
{
    // Each reply is preceded by<STX> and terminated by<CR>
    // <SP> indicates the ASCII space character, 0x20
    qDebug() << "Data received from X-ray source:" << data;

    if (data.isEmpty())
    {
        qDebug() << "Received empty data";
        return;
    }

    // Verify the data starts with STX (0x02) and ends with CR (0x0D)
    if (data.at(0) != 0x02)
    {
        qDebug() << "Invalid data format: missing STX";
        return;
    }

    if (data.at(data.size() - 1) != 0x0D)
    {
        qDebug() << "Invalid data format: missing CR";
        return;
    }

    // Extract the actual response data (remove STX and CR)
    QByteArray response = data.mid(1, data.size() - 2);
    qDebug() << "Parsed response:" << response;

    // 检查是否有线程正在等待响应
    if (!m_expectedResponsePrefix.isEmpty())
    {
        QString responseStr = QString::fromUtf8(response);
        // 检查响应是否匹配期望的前缀
        if (responseStr.startsWith(m_expectedResponsePrefix))
        {
            m_lastResponse = response;
            m_responseReceived = true;
            m_responseCondition.wakeAll();  // 唤醒等待的线程
            qDebug() << "Response matched expected prefix:" << m_expectedResponsePrefix;
        }
    }

    // 发送信号通知其他监听者
    emit dataReceived(response);
}
