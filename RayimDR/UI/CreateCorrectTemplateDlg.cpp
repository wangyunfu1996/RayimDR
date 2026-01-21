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
	auto gainPixmapitem = new QGraphicsPixmapItem();
	gainScence->addItem(gainPixmapitem);
	ui.graphicsView_GainImageView->setScene(gainScence);

	connect(ui.pushButton_GainInit, &QPushButton::clicked, this, &CreateCorrectTemplateDlg::GainInit);
	connect(ui.pushButton_GainAbort, &QPushButton::clicked, this, &CreateCorrectTemplateDlg::Abort);
	connect(ui.pushButton_GainStartAcq, &QPushButton::clicked, this, &CreateCorrectTemplateDlg::GainStartAcq);
	connect(ui.pushButton_GainSelectAll, &QPushButton::clicked, this, &CreateCorrectTemplateDlg::GainSelectAll);
	connect(ui.pushButton_GainGeneration, &QPushButton::clicked, this, &CreateCorrectTemplateDlg::GainGeneration);

	connect(&DET, &NDT1717MA::signalGainImageReceived, this, [this](int nCenterValue) {
		ui.lineEdit_GainCenterValue->setText(QString::number(nCenterValue));
		});
	connect(&DET, &NDT1717MA::signalAcqImageReceived, this, [this, gainPixmapitem](int idx) {
		QImage rawImage = DET.GetReceivedImage();
		gainPixmapitem->setPixmap(QPixmap::fromImage(rawImage));
		});
	connect(&DET, &NDT1717MA::signalGainImageSelected, this, [this](int nGainTotalFrames, int nValid) {
		ui.lineEdit_GainProgress->setText(QString("%1 / %2").arg(nValid).arg(nGainTotalFrames));
		});

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
		// 轮询检查是否完成（非阻塞）
		while (!DET.IsGainSelectComplete(std::move(f))) {
			// 做其他事情
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
		});

	// 创建 watcher 监听完成事件
	auto* watcher = new QFutureWatcher<void>(this);
	connect(watcher, &QFutureWatcher<void>::finished, this, [this, watcher]() {
		// 任务完成，恢复按钮可用性
		qDebug() << "GainSelectAll 采集完成";
		watcher->deleteLater();
		});
	watcher->setFuture(future);

	return true;
}

bool CreateCorrectTemplateDlg::GainGeneration()
{
	return DET.GainGeneration();
}

bool CreateCorrectTemplateDlg::Defect()
{
	ui.pushButton_Defect->setEnabled(false);

	// 移到后台线程执行
	auto future = QtConcurrent::run([this]() {
		DET.DefectInit();
		for (size_t idxGroup = 0; idxGroup < DET.nTotalGroup; idxGroup++)
		{
			DET.DefectStartAcq();
			QThread::msleep(1000);
			DET.DefectSelectAll(idxGroup);
			QThread::msleep(1000);
			DET.DefectForceDarkContinuousAcq(idxGroup);
		}
		DET.DefectGeneration();
	});

	// 创建 watcher 监听完成事件
	auto* watcher = new QFutureWatcher<void>(this);
	connect(watcher, &QFutureWatcher<void>::finished, this, [this, watcher]() {
		// 任务完成，恢复按钮可用性
		ui.pushButton_Defect->setEnabled(true);
		watcher->deleteLater();
	});
	watcher->setFuture(future);

	return true;
}

bool CreateCorrectTemplateDlg::DefectInit()
{
	return false;
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

