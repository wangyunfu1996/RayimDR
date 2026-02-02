#include "TcpClient.h"
#include <QDebug>
#include <QElapsedTimer>
#include <QTcpSocket>
#include <QThread>

TcpClient::TcpClient(QObject* parent) : QObject(parent), m_socket(nullptr)
{
    qDebug() << "Initializing TcpClient in thread:" << QThread::currentThread();

    // 设置 parent 为 this，确保在同一线程中
    m_socket = new QTcpSocket(this);

    connect(m_socket, &QTcpSocket::connected, this, &TcpClient::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &TcpClient::onDisconnected);
    connect(m_socket, &QTcpSocket::errorOccurred, this, &TcpClient::onError);

    qDebug() << "TcpClient initialized, socket thread:" << m_socket->thread();
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

QByteArray TcpClient::sendDataSync(const QByteArray& data, int timeout)
{
    if (!isConnected())
    {
        qDebug() << "Cannot send data: not connected";
        return QByteArray();
    }

    // 清空接收缓冲区
    m_socket->readAll();

    // 发送数据
    qDebug() << "Sending data synchronously, size:" << data.size() << "bytes, timeout:" << timeout << "ms";
    qint64 written = m_socket->write(data);
    if (written == -1)
    {
        qDebug() << "Failed to write data";
        return QByteArray();
    }

    m_socket->flush();
    qDebug() << "Data sent, written:" << written << "bytes, waiting for response...";

    QByteArray response;
    QElapsedTimer timer;
    timer.start();

    // 持续读取直到超时，处理分包问题
    // 策略：如果100ms内没有新数据到达，认为数据接收完整
    const int noDataInterval = 100;  // 无新数据的间隔时间(ms)

    while (timer.elapsed() < timeout)
    {
        int remainingTime = timeout - timer.elapsed();
        int waitTime = qMin(remainingTime, noDataInterval);

        if (m_socket->waitForReadyRead(waitTime))
        {
            QByteArray chunk = m_socket->readAll();
            response.append(chunk);
            qDebug() << "Received chunk, size:" << chunk.size() << "bytes, total:" << response.size() << "bytes";

            // 继续等待可能的后续数据包
            QElapsedTimer chunkTimer;
            chunkTimer.start();

            while (chunkTimer.elapsed() < noDataInterval)
            {
                if (m_socket->waitForReadyRead(noDataInterval - chunkTimer.elapsed()))
                {
                    QByteArray moreData = m_socket->readAll();
                    response.append(moreData);
                    qDebug() << "Received additional chunk, size:" << moreData.size()
                             << "bytes, total:" << response.size() << "bytes";
                    chunkTimer.restart();
                }
                else
                {
                    break;
                }
            }

            // 数据接收完整
            qDebug() << "Response received completely, total size:" << response.size() << "bytes";
            break;
        }
    }

    if (response.isEmpty())
    {
        qDebug() << "Timeout waiting for response:" << m_socket->errorString();
    }

    return response;
}

QByteArray TcpClient::sendDataSyncWithLength(const QByteArray& data, int expectedLength, int timeout)
{
    if (!isConnected())
    {
        qDebug() << "Cannot send data: not connected";
        return QByteArray();
    }

    // 清空接收缓冲区
    m_socket->readAll();

    // 发送数据
    qDebug() << "Sending data synchronously, size:" << data.size()
             << "bytes, expected response length:" << expectedLength << "bytes, timeout:" << timeout << "ms";
    qint64 written = m_socket->write(data);
    if (written == -1)
    {
        qDebug() << "Failed to write data";
        return QByteArray();
    }

    m_socket->flush();
    qDebug() << "Data sent, written:" << written << "bytes, waiting for response...";

    QByteArray response;
    QElapsedTimer timer;
    timer.start();

    // 持续读取直到收到期望长度的数据或超时
    while (response.size() < expectedLength && timer.elapsed() < timeout)
    {
        int remainingTime = timeout - timer.elapsed();
        if (m_socket->waitForReadyRead(remainingTime))
        {
            QByteArray chunk = m_socket->readAll();
            response.append(chunk);
            qDebug() << "Received chunk, size:" << chunk.size() << "bytes, total:" << response.size() << "/"
                     << expectedLength << "bytes";
        }
        else
        {
            qDebug() << "Timeout or error waiting for data:" << m_socket->errorString();
            break;
        }
    }

    if (response.size() >= expectedLength)
    {
        qDebug() << "Response received completely, size:" << response.size() << "bytes";
    }
    else
    {
        qDebug() << "Incomplete response, expected:" << expectedLength << "bytes, received:" << response.size()
                 << "bytes";
    }

    return response;
}

QByteArray TcpClient::sendDataSyncWithEndMarker(const QByteArray& data, const QByteArray& endMarker, int timeout)
{
    if (!isConnected())
    {
        qDebug() << "Cannot send data: not connected";
        return QByteArray();
    }

    // 清空接收缓冲区
    m_socket->readAll();

    // 发送数据
    qDebug() << "Sending data synchronously, size:" << data.size() << "bytes, data: " << data
             << ", end marker:" << endMarker.toHex() << ", timeout:" << timeout << "ms";
    qint64 written = m_socket->write(data);
    if (written == -1)
    {
        qDebug() << "Failed to write data";
        return QByteArray();
    }

    m_socket->flush();
    // qDebug() << "Data sent, written:" << written << "bytes, waiting for response...";

    QByteArray response;
    QElapsedTimer timer;
    timer.start();

    // 持续读取直到遇到结束标记或超时
    while (!response.contains(endMarker) && timer.elapsed() < timeout)
    {
        int remainingTime = timeout - timer.elapsed();
        if (m_socket->waitForReadyRead(remainingTime))
        {
            QByteArray chunk = m_socket->readAll();
            response.append(chunk);
            // qDebug() << "Received chunk, size:" << chunk.size() << "bytes, total:" << response.size() << "bytes";
        }
        else
        {
            qDebug() << "Timeout or error waiting for data:" << m_socket->errorString();
            break;
        }
    }

    if (response.contains(endMarker))
    {
        qDebug() << "Response received completely with end marker, size:" << response.size() << "bytes";
    }
    else
    {
        qDebug() << "End marker not found in response, size:" << response.size() << "bytes";
    }

    return response;
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

void TcpClient::onError()
{
    QString errorMsg = m_socket->errorString();
    qDebug() << "Error occurred:" << errorMsg;
    emit error(errorMsg);
}
