#include "TcpClient.h"
#include <QDebug>
#include <QTcpSocket>

TcpClient::TcpClient(QObject* parent) : QObject(parent), m_socket(nullptr)
{
    qDebug() << "Initializing TcpClient";
    m_socket = new QTcpSocket();

    connect(m_socket, &QTcpSocket::connected, this, &TcpClient::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &TcpClient::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &TcpClient::onReadyRead);
    connect(m_socket, &QTcpSocket::errorOccurred, this, &TcpClient::onError);
}

TcpClient::~TcpClient()
{
    qDebug() << "Destroying TcpClient";
    disconnectFromHost();
    delete m_socket;
    m_socket = nullptr;
}

bool TcpClient::connectToHost(const QString& host, quint16 port)
{
    qDebug() << "Attempting to connect to" << host << ":" << port;

    if (m_socket->state() == QAbstractSocket::ConnectedState || m_socket->state() == QAbstractSocket::ConnectingState)
    {
        qDebug() << "Already connected or connecting, state:" << m_socket->state();
        return false;
    }

    m_socket->connectToHost(host, port);
    bool success = m_socket->waitForConnected(5000);
    qDebug() << "Connect result:" << (success ? "SUCCESS" : "FAILED");
    return success;
}

void TcpClient::disconnectFromHost()
{
    if (m_socket->state() == QAbstractSocket::UnconnectedState)
    {
        qDebug() << "Already disconnected";
        return;
    }

    qDebug() << "Disconnecting from host";
    m_socket->disconnectFromHost();
    if (m_socket->state() != QAbstractSocket::UnconnectedState)
    {
        m_socket->waitForDisconnected(1000);
    }
    qDebug() << "Disconnected";
}

bool TcpClient::isConnected() const
{
    return m_socket->state() == QAbstractSocket::ConnectedState;
}

bool TcpClient::sendData(const QByteArray& data)
{
    if (!isConnected())
    {
        qDebug() << "Cannot send data: not connected";
        return false;
    }

    qDebug() << "Sending data, size:" << data.size() << "bytes";
    qint64 written = m_socket->write(data);
    if (written == -1)
    {
        qDebug() << "Failed to write data";
        return false;
    }

    m_socket->flush();
    qDebug() << "Data sent successfully, written:" << written << "bytes";
    return true;
}

void TcpClient::onConnected()
{
    qDebug() << "Connected to host";
    emit connected();
}

void TcpClient::onDisconnected()
{
    qDebug() << "Disconnected from host";
    emit disconnected();
}

void TcpClient::onReadyRead()
{
    QByteArray data = m_socket->readAll();
    qDebug() << "Data received, size:" << data.size() << "bytes"
             << " data:" << data;
    emit dataReceived(data);
}

void TcpClient::onError()
{
    QString errorMsg = m_socket->errorString();
    qDebug() << "Error occurred:" << errorMsg;
    emit error(errorMsg);
}
