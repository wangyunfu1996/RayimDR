#pragma once

#include <QObject>
#include <QNetworkInterface>
#include <QHostAddress>
#include <QList>
#include <QString>

#define xNetworkInfo XNetworkInfo::Instance()
#define pXNetworkInfo &XNetworkInfo::Instance()

/**
 * @brief 网络适配器信息查询类（单例模式）
 */
class XNetworkInfo : public QObject
{
    Q_OBJECT

private:
    XNetworkInfo(QObject* parent = nullptr);
    ~XNetworkInfo();

public:
    /**
     * @brief 获取单例实例
     * @return XNetworkInfo单例引用
     */
    static XNetworkInfo& Instance();

    /**
     * @brief 获取本机所有IPv4地址
     * @param includeLoopback 是否包含回环地址，默认false
     * @return IPv4地址列表
     */
    QList<QHostAddress> getAllIPv4Addresses(bool includeLoopback = false) const;

    /**
     * @brief 获取本机所有IPv6地址
     * @param includeLoopback 是否包含回环地址，默认false
     * @return IPv6地址列表
     */
    QList<QHostAddress> getAllIPv6Addresses(bool includeLoopback = false) const;

    /**
     * @brief 获取本机主机名
     * @return 主机名
     */
    QString getHostName() const;

    /**
     * @brief 将网络接口类型转换为可读字符串
     * @param type 网络接口类型
     * @return 类型描述字符串
     */
    static QString interfaceTypeToString(QNetworkInterface::InterfaceType type);
};
