#pragma once
#include "SerialPortController.h"
#include <QThread>
class VjxRay :public SerialPortController,public	QThread
{
	Q_OBJECT

public: 
	VjxRay();
	~VjxRay();
	//集成基类接口
public:
	//连接
	void run() override;
	bool connect(XRayControllerConnectParameter &parameter) override;
	//开光
	void openXRay() override;
	//关光
	void closeXRay() override;
	//包括预热、灯丝校准等完整
	void startUp()  override;
	///预热
   void warmUp()  override;

	//设置高压
	bool setVoltageInKV(double voltage) override;
	//设置功率
	bool setPowerInWatt(double power) override;
	//设置电流
	bool setCurrentInUA(double current) override;

	//重置过载
	bool resetOverload() override;
	//设置灯丝模式
	void setFilamentMode(int mode) override;//0=W, 1=S
	//设置焦点模式
	void setFocusMode(int mode) override;//0=S, 1=M, 2=L
	//查询光机各参数状态
	void queryDeviceStatus() override;

	//获取最小高压
    int getMinVoltage() const override;
	//获取最大高压
	int getMaxVoltage() override;
	//获取最小功率
	double getMinPower() const override;
	//获取最高功率
	double getMaxPower() override;
	//获取最小电流
	int getMinCurrent() const override;
	//获取最高电流
	int getMaxCurrent() const override;
	//获取初始高压
	int getDefaultVoltage() const override;
	//获取初始功率
	double getDefaultPower() const override;
	//获取初始电流
	int getDefaultCurrent() const override;
	//获取光机是否是电流控制
	bool isCurrentControl() const override;
//子类独有的接口
public:

protected:
	//子类自己的接口
	int warmupMpde = 1;//预热模式，0不需要预热，1：3~30天未开机，2：1~3个月未开机，3:3个月以上未开机
	void queryParameter();//电压、电流、温度、灯丝状态查询
	void queryFault();	//错误查询
	void queryHVStatus();//光机开关状态查询
	void queryWatchDogStatus();//看门狗状态查询
	//void query
	void clearFault();//错误清除
	void xRayStatus();//光机开关状态查询
	void watchDogSet(int IsEnable);//看门狗控制
	
	//解析数据
	void parseData(QString dataBuffer);

private:
	bool IsStart = false;//是否开启线程
	///用来标记当前的函数索引的
	int _functionIndex;
    //最低高电压(kv)
	const int  m_minVoltage = 100;
	//最高高压
	const int  m_maxVoltage = 200;
    //最低功率(w)
	const double m_minPower = 20;
	//最高功率
	double m_maxPower = 150;
	//最低电流（μA）
	const int  m_minCurrent = 200;
	//最高电流
	const int  m_maxCurrent = 1500;
	//初始功率
    const double  m_defaultPower = 20;
	//初始电流
	const int m_defaultCurrent = 200;
	//初始高压
    const int m_defaultVoltage = 100;
    //光机是否电流控制
	const bool m_IsCurrentControl = true;
	
   //暖机步骤
	const int warmProcedure = 10;
	//暖机时间
    QList<int> TimeOverThreeMonth ;
    QList<int>TimeOverOneMonth ;
    QList<int>TimeOverThreeDay;
	//暖机电压
    QList<int>WarmupVoltage ;
	//暖机电流
    QList<int>WarmupCurrent ;
};

