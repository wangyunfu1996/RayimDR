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
	this->setWindowTitle("探测器校正");
	this->setWindowButtonFlag(ElaAppBarType::StayTopButtonHint, false);
	this->setWindowButtonFlag(ElaAppBarType::MinimizeButtonHint, false);
	this->setWindowButtonFlag(ElaAppBarType::MaximizeButtonHint, false);
	this->resize(800, 400);
	this->moveToCenter();

	auto scene = new QGraphicsScene(ui.graphicsView_GainImageView);
	gainPixmapItem = new QGraphicsPixmapItem();
	scene->addItem(gainPixmapItem);
	ui.graphicsView_GainImageView->setScene(scene);

	scene = new QGraphicsScene(ui.graphicsView_DefectImageView);
	defectPixmapItem = new QGraphicsPixmapItem();
	scene->addItem(defectPixmapItem);
	ui.graphicsView_DefectImageView->setScene(scene);

	ui.spinBox_GainVoltage->setMinimum(0);
	ui.spinBox_GainVoltage->setMaximum(160);
	ui.spinBox_GainCurrent->setMinimum(0);
	ui.spinBox_GainCurrent->setMaximum(INT_MAX);

	ui.spinBox_DefectVoltage->setMinimum(0);
	ui.spinBox_DefectVoltage->setMaximum(160);
	ui.spinBox_DefectCurrent->setMinimum(0);
	ui.spinBox_DefectCurrent->setMaximum(INT_MAX);

	ui.lineEdit_OffsetProgress->setEnabled(false);

	connect(ui.pushButton_ToGain, &QPushButton::clicked, this, [this]() {
		ui.label_Tips->clear();
		ui.stackedWidget->setCurrentIndex(1);
		ui.pushButton_GainAbort->setEnabled(false);
		ui.pushButton_Gain->setEnabled(true);
		ui.spinBox_GainCurrent->setEnabled(false);
		ui.spinBox_GainVoltage->setEnabled(false);
		});

	connect(ui.pushButton_ToOffset, &QPushButton::clicked, this, [this]() {
		ui.stackedWidget->setCurrentIndex(0);
		});

	connect(ui.pushButton_ToDefect, &QPushButton::clicked, this, [this]() {
		ui.stackedWidget->setCurrentIndex(2);
		});
	connect(ui.pushButton_ReturnToGain, &QPushButton::clicked, this, [this]() {
		ui.stackedWidget->setCurrentIndex(1);
		});
	connect(ui.pushButton_Done, &QPushButton::clicked, this, [this]() {
		this->accept();
		});

	connect(ui.pushButton_OffsetGenerationStart, &QPushButton::clicked, this, &CreateCorrectTemplateDlg::Offset);
	connect(ui.pushButton_OffsetGenerationAbort, &QPushButton::clicked, this, &CreateCorrectTemplateDlg::Abort);

	connect(ui.pushButton_Gain, &QPushButton::clicked, this, &CreateCorrectTemplateDlg::Gain);
	connect(ui.pushButton_GainAbort, &QPushButton::clicked, this, &CreateCorrectTemplateDlg::Abort);

	connect(ui.pushButton_Defect, &QPushButton::clicked, this, &CreateCorrectTemplateDlg::Defect);
	connect(ui.pushButton_DefectAbort, &QPushButton::clicked, this, &CreateCorrectTemplateDlg::Abort);

	connect(this, &CreateCorrectTemplateDlg::signalTipsChanged, this, &CreateCorrectTemplateDlg::ShowTips, Qt::ConnectionType::UniqueConnection);

	ui.pushButton_OffsetGenerationStart->setEnabled(true);
	ui.pushButton_OffsetGenerationAbort->setEnabled(false);
	ui.pushButton_ToGain->setEnabled(false);

	ui.pushButton_DefectAbort->setEnabled(false);
	ui.lineEdit_DefectGroup->setEnabled(false);
	ui.lineEdit_DefectExceptedGray->setEnabled(false);
	ui.lineEdit_DefectCurrentGray->setEnabled(false);
	ui.spinBox_DefectVoltage->setEnabled(false);
	ui.spinBox_DefectCurrent->setEnabled(false);
	ui.lineEdit_DefectProgress->setEnabled(false);

	ui.label_Stage->setEnabled(false);
	connect(ui.stackedWidget, &QStackedWidget::currentChanged, this, [this](int idx) {
		if (idx == 0)
		{
			ui.label_Stage->setText("当前流程：暗场校正");
		}
		else if (idx == 1)
		{
			ui.label_Stage->setText("当前流程：亮场校正");
		}
		else if (idx == 2)
		{
			ui.label_Stage->setText("当前流程：缺陷校正");
		}
		});
	ui.stackedWidget->setCurrentIndex(0);

}

CreateCorrectTemplateDlg::~CreateCorrectTemplateDlg()
{}

void CreateCorrectTemplateDlg::Abort()
{
	DET.Abort();
}

void CreateCorrectTemplateDlg::Offset()
{
	ui.pushButton_OffsetGenerationStart->setEnabled(false);
	ui.pushButton_OffsetGenerationAbort->setEnabled(true);

	ui.lineEdit_OffsetProgress->clear();

	connect(&DET, &NDT1717MA::signalOffsetImageSelected, this, &CreateCorrectTemplateDlg::onOffsetImageSelected, Qt::ConnectionType::UniqueConnection);

	auto future = QtConcurrent::run([this]() {
		bool bRet = DET.OffsetGeneration();
		return bRet;
		});

	auto* watcher = new QFutureWatcher<bool>(this);
	connect(watcher, &QFutureWatcher<bool>::finished, this, [this, watcher]() {
		ui.pushButton_OffsetGenerationStart->setEnabled(true);
		ui.pushButton_OffsetGenerationAbort->setEnabled(false);
		bool bRet = watcher->future().result();
		qDebug() << "暗场校正流程结束，执行结果：" << bRet;
		if (bRet)
		{
			emit this->signalTipsChanged("暗场校正完成，请进行下一步校正");
			ui.pushButton_ToGain->setEnabled(true);
		}
		disconnect(&DET, &NDT1717MA::signalAcqImageReceived, this, &CreateCorrectTemplateDlg::onGainAcqImageReceived);
		watcher->deleteLater();
		});
	watcher->setFuture(future);
}

void CreateCorrectTemplateDlg::Gain()
{
	if (!DET.OffsetValid())
	{
		ShowTips("请先进行暗场校正");
		return;
	}

	connect(this, &CreateCorrectTemplateDlg::signalGainVoltageCurrentChanged, this, &CreateCorrectTemplateDlg::ModifyGainVoltageCurrent, Qt::ConnectionType::UniqueConnection);

	connect(&DET, &NDT1717MA::signalAcqImageReceived, this, &CreateCorrectTemplateDlg::onGainAcqImageReceived, Qt::ConnectionType::UniqueConnection);
	connect(&DET, &NDT1717MA::signalGainImageSelected, this, &CreateCorrectTemplateDlg::onGainImageSelected, Qt::ConnectionType::UniqueConnection);

	ui.pushButton_Gain->setEnabled(false);
	ui.pushButton_GainAbort->setEnabled(true);
	ui.label_Tips->clear();

	// 移到后台线程执行
	auto future = QtConcurrent::run([this]() {
		nCurrentGray = 0;
		emit this->signalTipsChanged("正在执行初始化");
		bool bRet = DET.GainInit();
		if (!bRet)
		{
			emit this->signalTipsChanged("初始化失败！");
			return false;
		}

		int nVoltage = DET.nGainVoltage;
		int nBeginCurrent = DET.nGainBeginCurrent;
		int nCurrent = nBeginCurrent;
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

		emit this->signalGainVoltageCurrentChanged(nVoltage, nBeginCurrent);
		// 等待 灰度值达到要求
		while (nCurrentGray < DET.nGainExpectedGray)
		{
			qDebug() << "nCurrentGray: " << nCurrentGray
				<< " nGainExpectedGray: " << DET.nGainExpectedGray
				<< " nVoltage: " << nVoltage
				<< " nBeginCurrent: " << nBeginCurrent
				<< " nCurrent: " << nCurrent;

			// 调整电压电流
			if (nCurrent > 1000)
			{
				qDebug() << "射线源已到达最大功率";
				break;
			}

			nCurrent += 100;
			emit this->signalGainVoltageCurrentChanged(nVoltage, nCurrent);
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
		return true;
		});

	// 创建 watcher 监听完成事件
	auto* watcher = new QFutureWatcher<bool>(this);
	connect(watcher, &QFutureWatcher<bool>::finished, this, [this, watcher]() {
		// 任务完成，恢复按钮可用性
		ui.pushButton_Gain->setEnabled(true);
		ui.pushButton_GainAbort->setEnabled(false);
		bool bRet = watcher->future().result();
		qDebug() << "亮场校正流程结束，执行结果：" << bRet;
		if (bRet)
		{
			emit this->signalTipsChanged("亮场校正完成，请进行下一步校正");
		}
		disconnect(&DET, &NDT1717MA::signalAcqImageReceived, this, &CreateCorrectTemplateDlg::onGainAcqImageReceived);
		watcher->deleteLater();
		});
	watcher->setFuture(future);
}

void CreateCorrectTemplateDlg::Defect()
{
	if (!DET.GainValid())
	{
		ShowTips("请先进行亮场校正");
		return;
	}

	connect(this, &CreateCorrectTemplateDlg::signalDefectVoltageCurrentChanged, this, &CreateCorrectTemplateDlg::ModifyDefectVoltageCurrent, Qt::ConnectionType::UniqueConnection);
	connect(this, &CreateCorrectTemplateDlg::signalDefectGroupChanged, this, &CreateCorrectTemplateDlg::onDefectGroupChanged, Qt::ConnectionType::UniqueConnection);
	connect(&DET, &NDT1717MA::signalAcqImageReceived, this, &CreateCorrectTemplateDlg::onDefectAcqImageReceived, Qt::ConnectionType::UniqueConnection);
	connect(&DET, &NDT1717MA::signalDefectImageSelected, this, &CreateCorrectTemplateDlg::onDefectImageSelected, Qt::ConnectionType::UniqueConnection);

	ui.pushButton_Defect->setEnabled(false);
	ui.pushButton_DefectAbort->setEnabled(true);
	ui.label_Tips->clear();

	// 移到后台线程执行
	auto future = QtConcurrent::run([this]() {
		nCurrentGray = 0;
		emit this->signalTipsChanged("正在执行初始化");
		if (!DET.DefectInit())
		{
			emit this->signalTipsChanged("初始化失败！");
			return false;
		}

		for (int idxGroup = 0; idxGroup < DET.nTotalGroup; idxGroup++)
		{
			emit this->signalDefectGroupChanged(idxGroup, DET.nTotalGroup);
			int nVoltage = DET.nDefectVoltages[idxGroup];
			int nBeginCurrent = DET.nDefectBeginCurrents[idxGroup];
			int nCurrent = nBeginCurrent;
			int nExceptedGray = DET.nDefectExpectedGrays[idxGroup];
			emit this->signalTipsChanged("等待射线源开启");
			QThread::msleep(5000);
			emit this->signalTipsChanged("射线源已开启");

			if (!DET.DefectStartAcq())
			{
				return false;
			}

			while (nCurrentGray < DET.nGainExpectedGray)
			{
				qDebug() << "idxGroup: " << idxGroup
					<< " nExceptedGray: " << nExceptedGray
					<< " nCurrentGray: " << nCurrentGray
					<< " nVoltage: " << nVoltage
					<< " nCurrent: " << nCurrent;

				// 调整电压电流
				if (nCurrent > 500)
				{
					qDebug() << "射线源已到达最大功率";
					break;
				}
				nCurrent += 100;
				emit this->signalDefectVoltageCurrentChanged(nVoltage, nCurrent);
				QThread::msleep(2000);
			}

			if (!DET.DefectSelectAll(idxGroup))
			{
				return false;
			}

			emit this->signalTipsChanged("等待射线源关闭");
			QThread::msleep(2000);
			emit this->signalTipsChanged("射线源已关闭");

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
		ui.pushButton_DefectAbort->setEnabled(false);
		bool bRet = watcher->future().result();
		qDebug() << "一键缺陷矫正流程结束，执行结果：" << bRet;
		if (bRet)
		{
			emit this->signalTipsChanged("陷矫正流程完成");
		}
		disconnect(&DET, &NDT1717MA::signalAcqImageReceived, this, &CreateCorrectTemplateDlg::onDefectAcqImageReceived);
		watcher->deleteLater();
		});
	watcher->setFuture(future);
}

void CreateCorrectTemplateDlg::ShowTips(const QString& msg)
{
	ui.label_Tips->setText(msg);
}

void CreateCorrectTemplateDlg::ModifyGainVoltageCurrent(int voltage, int current)
{
	ui.spinBox_GainVoltage->setValue(voltage);
	ui.spinBox_GainCurrent->setValue(current);
}

void CreateCorrectTemplateDlg::ModifyDefectVoltageCurrent(int voltage, int current)
{
	ui.spinBox_DefectVoltage->setValue(voltage);
	ui.spinBox_DefectCurrent->setValue(current);
}

void CreateCorrectTemplateDlg::onOffsetImageSelected(int nTotal, int nValid)
{
	ui.lineEdit_OffsetProgress->setText(QString("%1 / %2").arg(nValid).arg(nTotal));
}

void CreateCorrectTemplateDlg::onGainAcqImageReceived(int idx)
{
	QImage rawImage = DET.GetReceivedImage();
	gainPixmapItem->setPixmap(QPixmap::fromImage(rawImage).scaled(ui.graphicsView_GainImageView->width() - 5,
		ui.graphicsView_GainImageView->height() - 5));
	int nGray = DET.GetGrayOfReceivedImage();
	ui.lineEdit_GainCenterValue->setText(QString::number(nGray));
}

void CreateCorrectTemplateDlg::onGainImageSelected(int nTotal, int nValid)
{
	ui.lineEdit_GainProgress->setText(QString("%1 / %2").arg(nValid).arg(nTotal));
}

void CreateCorrectTemplateDlg::onDefectAcqImageReceived(int idx)
{
	QImage rawImage = DET.GetReceivedImage();
	defectPixmapItem->setPixmap(QPixmap::fromImage(rawImage).scaled(ui.graphicsView_DefectImageView->width() - 5,
		ui.graphicsView_DefectImageView->height() - 5));
	int nGray = DET.GetGrayOfReceivedImage();
	ui.lineEdit_DefectCurrentGray->setText(QString::number(nGray));
}

void CreateCorrectTemplateDlg::onDefectGroupChanged(int groupIdx, int nTotalGroup)
{
	ui.lineEdit_DefectExceptedGray->setText(QString::number(DET.nDefectExpectedGrays[groupIdx]));
	ui.lineEdit_DefectGroup->setText(QString("%1 / %2").arg(groupIdx + 1).arg(nTotalGroup));
}

void CreateCorrectTemplateDlg::onDefectImageSelected(int nTotal, int nValid)
{
	ui.lineEdit_DefectProgress->setText(QString("%1 / %2").arg(nValid).arg(nTotal));
}
