#pragma once

#include <QObject>

class QTcpSocket;

class TcpClient : public QObject
{
    Q_OBJECT

public:
    explicit TcpClient(QObject* parent = nullptr);
    ~TcpClient();

    // 连接管理
    bool connectToHost(const QString& host, quint16 port);
    void disconnectFromHost();
    bool isConnected() const;

    // 同步发送并接收数据
    // timeout: 等待响应的超时时间(毫秒)，默认5000ms
    // 返回值: 接收到的数据，如果失败或超时则返回空QByteArray
    QByteArray sendDataSync(const QByteArray& data, int timeout = 5000);

    // 同步发送并接收指定长度的数据
    // expectedLength: 期望接收的数据长度，接收到指定长度后立即返回
    // timeout: 等待响应的超时时间(毫秒)
    QByteArray sendDataSyncWithLength(const QByteArray& data, int expectedLength, int timeout = 5000);

    // 同步发送并接收数据，直到遇到结束符
    // endMarker: 数据包结束标记
    // timeout: 等待响应的超时时间(毫秒)
    QByteArray sendDataSyncWithEndMarker(const QByteArray& data, const QByteArray& endMarker, int timeout = 5000);

signals:
    void connected();
    void disconnected();
    void error(const QString& errorMsg);

private slots:
    void onConnected();
    void onDisconnected();
    void onError();

private:
    QTcpSocket* m_socket;
};
