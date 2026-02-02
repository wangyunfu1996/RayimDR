#pragma once

#include <QObject>
#include <QString>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QEventLoop>
#include <QTimer>

class TcpClient;

#define xRaySource IXS120BP120P366::Instance()
#define pXRaySource &IXS120BP120P366::Instance()

class IXS120BP120P366 : public QObject
{
    Q_OBJECT

private:
    explicit IXS120BP120P366(QObject* parent = nullptr);
    ~IXS120BP120P366();

    IXS120BP120P366(const IXS120BP120P366&) = delete;
    IXS120BP120P366& operator=(const IXS120BP120P366&) = delete;

public:
    static IXS120BP120P366& Instance();

    bool connectToSource(const QString& host, quint16 port);
    void disconnectFromSource();
    bool isConnected() const;

    bool sendCommand(const std::string& cmd, const std::string& param = "");
    bool sendCmdAndWaitForResponse(const std::string& cmd, const std::string& param, QString& response,
                                int timeoutMs = 2000);
    bool setVoltage(int kV);
    bool setCurrent(int uA);

signals:
    void connected();
    void disconnected();
    void error(const QString& errorMsg);
    void dataReceived(const QByteArray& data);

private slots:
    void onTcpConnected();
    void onTcpDisconnected();
    void onTcpError(const QString& errorMsg);
    void onTcpDataReceived(const QByteArray& data);

private:
    TcpClient* m_tcpClient;
    bool m_connected;
    QThread m_thread;

    // 用于阻塞等待响应
    QMutex m_responseMutex;
    QWaitCondition m_responseCondition;
    QByteArray m_lastResponse;
    bool m_responseReceived;
    QString m_expectedResponsePrefix;  // 期望的响应前缀，用于识别正确的响应
};
