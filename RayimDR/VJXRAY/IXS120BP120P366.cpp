#include "IXS120BP120P366.h"
#include "TcpClient.h"
#include <QDebug>

#pragma warning(disable : 4996)  // disable deprecated function warning

namespace
{
const int CMD_MAX_LENGTH = 48;
const std::string CMD_PREFIX = "\x02";  // STX
const std::string CMD_SUFFIX = "\x0D";  // CR

const std::string CMD_REPORT_MODEL_NUMBER = CMD_PREFIX + "MNUM" + CMD_SUFFIX;
}  // namespace

IXS120BP120P366::IXS120BP120P366(QObject* parent) : QObject(parent), m_tcpClient(nullptr), m_connected(false)
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

bool IXS120BP120P366::setVoltage(int kV)
{
    return false;
}

bool IXS120BP120P366::setCurrent(int mA)
{
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

    emit dataReceived(response);
}
