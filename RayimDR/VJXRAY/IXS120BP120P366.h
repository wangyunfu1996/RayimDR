#pragma once

#include <QObject>
#include <QString>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QEventLoop>
#include <QTimer>

class TcpClient;

// X-ray source status structure
struct XRaySourceStatus
{
    double voltage;          // Current voltage in kV
    double current;          // Current in uA
    double temperature;      // Temperature in Celsius
    double filamentCurrent;  // Filament current in A
    double vdc;              // kVoltage in VDC
    std::vector<int> faultBits;
    int interlock;  // Interlock status
};

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

    bool setVoltage(int kV);
    bool setCurrent(int uA);
    bool startXRay();
    bool stopXRay();
    void clearErr();
    bool xRayIsOn();
    void setIsPreheat(bool preheat);
    bool isPreheat() const;

    int getPTST();

    QByteArray sendDataSyncWithEndMarker(const QByteArray& data, const QByteArray& endMarker, int timeout = 5000);

    // Status query control
    void startStatusQuery(int intervalMs = 1000);
    void stopStatusQuery();
    bool isStatusQueryRunning() const;
    XRaySourceStatus getCurrentStatus() const;

signals:
    void connected();
    void disconnected();
    void xrayError(const QString& errorMsg);
    void xrayErrorCleared();  // 错误已清除信号
    void statusUpdated(const XRaySourceStatus& status);

private slots:
    void onTcpConnected();
    void onTcpDisconnected();
    void onTcpError(const QString& errorMsg);
    void onQueryStatus();

private:
    void parseMONResponse(const QString& response);
    void parseFTLResponse(const QString& response);

private:
    TcpClient* m_tcpClient{nullptr};
    bool m_connected;
    QThread m_thread;
    // Status query
    QTimer* m_statusQueryTimer{nullptr};
    XRaySourceStatus m_currentStatus;
    mutable QMutex m_statusMutex;
    bool m_statusQueryEnabled{false};
    bool m_isPreheat{false};
};
