#include "CreateCorrectTemplateDlg.h"

#include <qgraphicsscene.h>
#include <qgraphicsitem.h>
#include <QtConcurrent/QtConcurrent>
#include <QFutureWatcher>

#include "../IRayDetector/NDT1717MA.h"

CreateCorrectTemplateDlg::CreateCorrectTemplateDlg(QWidget* parent)
	: ElaDialog(parent)
{
	ui.setupUi(this);

	connect(ui.pushButton_OffsetGenerationStart, &QPushButton::clicked, this, &CreateCorrectTemplateDlg::OffsetGeneration);
	connect(ui.pushButton_OffsetGenerationAbort, &QPushButton::clicked, this, &CreateCorrectTemplateDlg::Abort);

	auto gainScence = new QGraphicsScene(ui.graphicsView_GainImageView);
	gainPixmapItem = new QGraphicsPixmapItem();
	gainScence->addItem(gainPixmapItem);
	ui.graphicsView_GainImageView->setScene(gainScence);
	ui.spinBox_GainTargetCurrent->setMaximum(0);
	ui.spinBox_GainTargetCurrent->setMaximum(INT_MAX);

	connect(ui.pushButton_Gain, &QPushButton::clicked, this, &CreateCorrectTemplateDlg::Gain);
	connect(ui.pushButton_Defect, &QPushButton::clicked, this, &CreateCorrectTemplateDlg::Defect);
	connect(ui.pushButton_DefectAbort, &QPushButton::clicked, this, &CreateCorrectTemplateDlg::Abort);
}

CreateCorrectTemplateDlg::~CreateCorrectTemplateDlg()
{}

void CreateCorrectTemplateDlg::Abort()
{
	DET.Abort();
}

bool CreateCorrectTemplateDlg::OffsetGeneration()
{
	return 0 == DET.OffsetGeneration();
}

void CreateCorrectTemplateDlg::Gain()
{
	connect(this, &CreateCorrectTemplateDlg::signalTipsChanged, this, &CreateCorrectTemplateDlg::ShowTips, Qt::ConnectionType::UniqueConnection);
	connect(this, &CreateCorrectTemplateDlg::signalGainVoltageCurrentChanged, this, &CreateCorrectTemplateDlg::MidifyGainVoltageCurrent, Qt::ConnectionType::UniqueConnection);

	connect(&DET, &NDT1717MA::signalGainImageReceived, this, [this](int nCenterValue) {
		nCurrentGray = nCenterValue;
		ui.lineEdit_GainCenterValue->setText(QString::number(nCenterValue));
		}, Qt::ConnectionType::UniqueConnection);
	connect(&DET, &NDT1717MA::signalAcqImageReceived, this, [this](int idx) {
		QImage rawImage = DET.GetReceivedImage();
		gainPixmapItem->setPixmap(QPixmap::fromImage(rawImage));
		}, Qt::ConnectionType::UniqueConnection);
	connect(&DET, &NDT1717MA::signalGainImageSelected, this, [this](int nGainTotalFrames, int nValid) {
		ui.lineEdit_GainProgress->setText(QString("%1 / %2").arg(nValid).arg(nGainTotalFrames));
		}, Qt::ConnectionType::UniqueConnection);

	ui.pushButton_Gain->setEnabled(false);

	// 移到后台线程执行
	auto future = QtConcurrent::run([this]() {
		emit this->signalTipsChanged("正在执行初始化");
		bool bRet = DET.GainInit();
		if (!bRet)
		{
			emit this->signalTipsChanged("初始化失败！");
			return false;
		}

		emit this->signalTipsChanged("正在等待射线源开启");
		// 等待射线源已开启
		QThread::msleep(5000);
		emit this->signalTipsChanged("射线源已开启");

		emit this->signalTipsChanged("正在自动调整电压电流，并采集亮场");
		bRet = DET.GainStartAcq();
		if (!bRet)
		{
			emit this->signalTipsChanged("开启采集失败！");
			return false;
		}

		int nGainCurrentStart = DET.nGainCurrentStart;
		int nGainCurrentCurrent = DET.nGainCurrentStart;
		int nGainCurrentEnd = DET.nGainCurrentEnd;
		int nGainCurrentStep = DET.nGainCurrentStep;

		// 等待 灰度值达到要求
		while (nCurrentGray < DET.nGainExpectedGray)
		{
			qDebug() << "nCurrentGray: " << nCurrentGray
				<< " nGainCurrentStart: " << nGainCurrentStart
				<< " nGainCurrentCurrent: " << nGainCurrentCurrent
				<< " nGainCurrentEnd: " << nGainCurrentEnd
				<< " nGainCurrentStep: " << nGainCurrentStep;

			// 调整电压电流
			if (nGainCurrentCurrent > nGainCurrentEnd)
			{
				qDebug() << "射线源已到达最大功率";
				break;
			}

			nGainCurrentCurrent += nGainCurrentStep;
			emit this->signalGainVoltageCurrentChanged(DET.nGainSuggestedKV, nGainCurrentCurrent);
			QThread::msleep(2000);
		}

		bRet = DET.GainSelectAll().get();
		if (!bRet)
		{
			emit this->signalTipsChanged("获取合适的图像失败！");
			return false;
		}
		emit this->signalTipsChanged("亮场采集结束");

		emit this->signalTipsChanged("正在执行计算");
		bRet = DET.GainGeneration();
		if (!bRet)
		{
			emit this->signalTipsChanged("计算失败！");
			return false;
		}
		emit this->signalTipsChanged("亮场校正完成，请进行下一步校正");

		return true;
		});

	// 创建 watcher 监听完成事件
	auto* watcher = new QFutureWatcher<bool>(this);
	connect(watcher, &QFutureWatcher<bool>::finished, this, [this, watcher]() {
		// 任务完成，恢复按钮可用性
		ui.pushButton_Gain->setEnabled(true);
		qDebug() << "亮场校正流程结束，执行结果：" << watcher->future().result();
		watcher->deleteLater();
		});
	watcher->setFuture(future);

}

bool CreateCorrectTemplateDlg::GainInit()
{
	return 0 == DET.GainInit();
}

bool CreateCorrectTemplateDlg::GainStartAcq()
{
	return DET.GainStartAcq();
}

bool CreateCorrectTemplateDlg::GainSelectAll()
{
	// 移到后台线程执行
	auto future = QtConcurrent::run([this]() {
		// 启动异步任务
		auto f = DET.GainSelectAll();
		f.wait();
		});

	// 创建 watcher 监听完成事件
	auto* watcher = new QFutureWatcher<void>(this);
	connect(watcher, &QFutureWatcher<void>::finished, this, [this, watcher]() {
		// 任务完成，恢复按钮可用性
		qDebug() << "亮场采集完成";
		watcher->deleteLater();
		});
	watcher->setFuture(future);

	return true;
}

bool CreateCorrectTemplateDlg::GainGeneration()
{
	return DET.GainGeneration();
}

void CreateCorrectTemplateDlg::Defect()
{
	ui.pushButton_Defect->setEnabled(false);

	// 移到后台线程执行
	auto future = QtConcurrent::run([this]() {
		if (!DET.DefectInit())
		{
			return false;
		}

		for (size_t idxGroup = 0; idxGroup < DET.nTotalGroup; idxGroup++)
		{
			// 等待射线源已开启
			QThread::msleep(2000);
			qDebug() << "射线源已开启";

			if (!DET.DefectStartAcq())
			{
				return false;
			}

			if (!DET.DefectSelectAll(idxGroup))
			{
				return false;
			}

			// 等待射线源已关闭
			QThread::msleep(2000);
			qDebug() << "射线源已关闭";

			if (!DET.DefectForceDarkContinuousAcq(idxGroup))
			{
				return false;
			}
		}

		return DET.DefectGeneration();
		});

	// 创建 watcher 监听完成事件
	auto* watcher = new QFutureWatcher<bool>(this);
	connect(watcher, &QFutureWatcher<bool>::finished, this, [this, watcher]() {
		// 任务完成，恢复按钮可用性
		ui.pushButton_Defect->setEnabled(true);
		qDebug() << "一键缺陷矫正流程结束，执行结果：" << watcher->future().result();
		watcher->deleteLater();
		});
	watcher->setFuture(future);
}

bool CreateCorrectTemplateDlg::DefectInit()
{
	bool bRet = DET.DefectInit();
	QThread::msleep(500);
	return bRet;
}

bool CreateCorrectTemplateDlg::DefectStartAcq()
{
	return false;
}

bool CreateCorrectTemplateDlg::DefectSelectAll()
{
	return false;
}

bool CreateCorrectTemplateDlg::DefectGeneration()
{
	return false;
}

void CreateCorrectTemplateDlg::ShowTips(const QString& msg)
{
	ui.label_Tips->setText(msg);
}

void CreateCorrectTemplateDlg::MidifyGainVoltageCurrent(int voltage, int current)
{
	ui.lineEdit_GainSuggestedKV->setText(QString::number(voltage));
	ui.spinBox_GainTargetCurrent->setValue(current);
}

