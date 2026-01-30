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

    // 数据发送
    bool sendData(const QByteArray& data);

signals:
    void connected();
    void disconnected();
    void error(const QString& errorMsg);
    void dataReceived(const QByteArray& data);

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onError();

private:
    QTcpSocket* m_socket;
};
