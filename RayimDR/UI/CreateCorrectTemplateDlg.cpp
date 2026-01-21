#include "CreateCorrectTemplateDlg.h"

#include <qgraphicsscene.h>
#include <qgraphicsitem.h>

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
	connect(&DET, &NDT1717MA::signalGaimImageSelected, this, [this](int nGainTotalFrames, int nValid) {
		ui.lineEdit_GainProgress->setText(QString("%1 / %2").arg(nValid).arg(nGainTotalFrames));
		});
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
	return DET.GainSelectAll();
}

bool CreateCorrectTemplateDlg::GainGeneration()
{
	return DET.GainGeneration();
}

