#include "IXS120BP120P366.h"
#include "TcpClient.h"
#include <QDebug>

IXS120BP120P366::IXS120BP120P366(QObject* parent) : QObject(parent), m_tcpClient(nullptr), m_connected(false)
{
    qDebug() << "Initializing IXS120BP120P366";

    m_tcpClient = new TcpClient();
    m_tcpClient->moveToThread(&m_thread);

    connect(m_tcpClient, &TcpClient::connected, this, &IXS120BP120P366::onTcpConnected);
    connect(m_tcpClient, &TcpClient::disconnected, this, &IXS120BP120P366::onTcpDisconnected);
    connect(m_tcpClient, &TcpClient::error, this, &IXS120BP120P366::onTcpError);
    connect(m_tcpClient, &TcpClient::dataReceived, this, &IXS120BP120P366::onTcpDataReceived);

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

bool IXS120BP120P366::sendCommand(const QByteArray& command)
{
    if (!isConnected())
    {
        qDebug() << "Cannot send command: not connected";
        return false;
    }

    qDebug() << "Sending command:" << command;
    return m_tcpClient->sendData(command);
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
    qDebug() << "Data received from X-ray source:" << data;
}
