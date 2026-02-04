#include "XNetworkInfo.h"

#include <QDebug>
#include <QHostInfo>

// ============================================================================
// Constructor / Destructor
// ============================================================================

XNetworkInfo::XNetworkInfo(QObject* parent) : QObject(parent)
{
    qDebug() << "[XNetworkInfo] Network info utility initialized";
}

XNetworkInfo::~XNetworkInfo()
{
    qDebug() << "[XNetworkInfo] Network info utility destroyed";
}

// ============================================================================
// Singleton Instance
// ============================================================================

XNetworkInfo& XNetworkInfo::Instance()
{
    static XNetworkInfo instance;
    return instance;
}

// ============================================================================
// Public Methods
// ============================================================================

QList<QHostAddress> XNetworkInfo::getAllIPv4Addresses(bool includeLoopback) const
{
    QList<QHostAddress> ipv4Addresses;
    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();

    for (const QNetworkInterface& interface : interfaces)
    {
        // qDebug() << interface.name();
        // qDebug() << interface.humanReadableName();
        // qDebug() << interface.flags();
        // qDebug() << interface.type();
        // qDebug() << interface.hardwareAddress();
        // qDebug() << interface.addressEntries();
        // qDebug() << interface.isValid();

        // 只获取已启用(IsUp)的网络接口
        if (!interface.flags().testFlag(QNetworkInterface::IsUp))
        {
            continue;
        }

        // 如果不包含回环地址，则跳过回环接口
        if (!includeLoopback && interface.flags().testFlag(QNetworkInterface::IsLoopBack))
        {
            continue;
        }

        QList<QNetworkAddressEntry> entries = interface.addressEntries();
        for (const QNetworkAddressEntry& entry : entries)
        {
            QHostAddress address = entry.ip();
            if (address.protocol() == QAbstractSocket::IPv4Protocol)
            {
                ipv4Addresses.append(address);
            }
        }
    }

    return ipv4Addresses;
}

QList<QHostAddress> XNetworkInfo::getAllIPv6Addresses(bool includeLoopback) const
{
    QList<QHostAddress> ipv6Addresses;
    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();

    for (const QNetworkInterface& interface : interfaces)
    {
        // 只获取已启用(IsUp)的网络接口
        if (!interface.flags().testFlag(QNetworkInterface::IsUp))
        {
            continue;
        }

        // 如果不包含回环地址，则跳过回环接口
        if (!includeLoopback && interface.flags().testFlag(QNetworkInterface::IsLoopBack))
        {
            continue;
        }

        QList<QNetworkAddressEntry> entries = interface.addressEntries();
        for (const QNetworkAddressEntry& entry : entries)
        {
            QHostAddress address = entry.ip();
            if (address.protocol() == QAbstractSocket::IPv6Protocol)
            {
                ipv6Addresses.append(address);
            }
        }
    }

    return ipv6Addresses;
}

QString XNetworkInfo::getHostName() const
{
    return QHostInfo::localHostName();
}

QString XNetworkInfo::interfaceTypeToString(QNetworkInterface::InterfaceType type)
{
    switch (type)
    {
        case QNetworkInterface::Unknown:
            return "Unknown";
        case QNetworkInterface::Loopback:
            return "Loopback";
        case QNetworkInterface::Ethernet:
            return "Ethernet";
        case QNetworkInterface::Wifi:
            return "WiFi";
        case QNetworkInterface::Virtual:
            return "Virtual";
        default:
            return QString("Other(%1)").arg(static_cast<int>(type));
    }
}
