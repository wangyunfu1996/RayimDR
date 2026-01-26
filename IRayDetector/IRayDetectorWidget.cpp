#include "IRayDetectorWidget.h"
#include "ui_IRayDetectorWidget.h"

#include <qdebug.h>
#include <qmessagebox.h>

#include <QtConcurrent/QtConcurrent>
#include <QFuture>
#include <QFutureWatcher>

#include "../IRayDetector/NDT1717MA.h"
#include "../IRayDetector/TiffHelper.h"

IRayDetectorWidget::IRayDetectorWidget(QWidget* parent)
	: QMainWindow(parent), ui(new Ui::IRayDetectorWidgetClass)
{
	ui->setupUi(this);

	connect(ui->pushButton_connect, &QPushButton::clicked, this, [this]() {
		if (!DET.Initialize())
		{
			qDebug() << "探测器初始化失败！";
			DET.DeInitialize();
		}
		else
		{
			DET.UpdateMode("Mode5");
			int sw_offset{ -1 };
			int sw_gain{ -1 };
			int sw_defect{ -1 };
			DET.GetCurrentCorrectOption(sw_offset, sw_gain, sw_defect);

			sw_offset = 1;
			sw_gain = 1;
			sw_defect = 1;
			DET.SetCorrectOption(sw_offset, sw_gain, sw_defect);

			DET.SetPreviewImageEnable(0);
		}
		});

	connect(ui->pushButton_disconnect, &QPushButton::clicked, this, [this]() {
		DET.DeInitialize();
		});

	connect(ui->pushButton_Abort, &QPushButton::clicked, this, [this]() {
		DET.Abort();
		});

	DET.QueryStatus();
	auto status = DET.Status();
	ui->checkBox_offset->setChecked(status.SW_PreOffset);
	ui->checkBox_gain->setChecked(status.SW_Gain);
	ui->checkBox_defect->setChecked(status.SW_Defect);

	connect(&DET, &NDT1717MA::signalStatusChanged, this, [this]() {
		auto status = DET.Status();
		ui->checkBox_offset->setChecked(status.SW_PreOffset);
		ui->checkBox_gain->setChecked(status.SW_Gain);
		ui->checkBox_defect->setChecked(status.SW_Defect);
		});

	auto updateCorrectOption = [this]() {
		int sw_offset = ui->checkBox_offset->isChecked();
		int sw_gain = ui->checkBox_gain->isChecked();
		int sw_defect = ui->checkBox_defect->isChecked();
		DET.SetCorrectOption(sw_offset, sw_gain, sw_defect);
	};

	connect(ui->checkBox_offset, &QCheckBox::toggled, this, [this, updateCorrectOption](bool checked) {
		Q_UNUSED(checked);
		updateCorrectOption();
		});
	connect(ui->checkBox_gain, &QCheckBox::toggled, this, [this, updateCorrectOption](bool checked) {
		Q_UNUSED(checked);
		updateCorrectOption();
		});
	connect(ui->checkBox_defect, &QCheckBox::toggled, this, [this, updateCorrectOption](bool checked) {
		Q_UNUSED(checked);
		updateCorrectOption();
		});

	connect(ui->pushButton_nVal, &QPushButton::clicked, this, [this]() {
		ui->lineEdit_msg->clear();
		ui->lineEdit_nVal->clear();

		int nAttrID = ui->lineEdit_nAttrID->text().toInt();
		int nVal{ -1 };
		if (DET.GetAttr(nAttrID, nVal))
		{
			ui->lineEdit_nVal->setText(QString::number(nVal));
		}
		else
		{
			ui->lineEdit_msg->setText("读取错误！");
		}
		});

	connect(ui->pushButton_fVal, &QPushButton::clicked, this, [this]() {
		ui->lineEdit_msg->clear();
		ui->lineEdit_fVal->clear();

		int nAttrID = ui->lineEdit_nAttrID->text().toInt();
		float fVal{ -1.0 };
		if (DET.GetAttr(nAttrID, fVal))
		{
			ui->lineEdit_fVal->setText(QString::number(fVal));
		}
		else
		{
			ui->lineEdit_msg->setText("读取错误！");
		}
		});

	connect(ui->pushButton_strVal, &QPushButton::clicked, this, [this]() {
		ui->lineEdit_msg->clear();
		ui->lineEdit_strVal->clear();

		int nAttrID = ui->lineEdit_nAttrID->text().toInt();
		std::string strVal;
		if (DET.GetAttr(nAttrID, strVal))
		{
			ui->lineEdit_strVal->setText(QString::fromStdString(strVal));
		}
		else
		{
			ui->lineEdit_msg->setText("读取错误！");
		}
		});

	connect(ui->pushButton_clearAcq, &QPushButton::clicked, this, [this]() {
		DET.ClearAcq();
		});

	connect(ui->pushButton_StartAcq, &QPushButton::clicked, this, [this]() {
		int state{ -1 };
		DET.GetDetectorState(state);
		if (state == 2)
		{
			QMessageBox::warning(nullptr,
				"警告",
				"请等待当前采集完成，或取消当前采集任务！",
				QMessageBox::Ok);
			return;
		}

		DET.StartAcq();

		});

	connect(ui->pushButton_StopAcq, &QPushButton::clicked, this, [this]() {
		DET.StopAcq();
		});

	connect(ui->pushButton_readImage, &QPushButton::clicked, this, [this]() {
		static QLabel* label = nullptr;
		if (nullptr == label)
		{
			qDebug() << "新建 QLabel";
			label = new QLabel;
		}

		qDebug() << "开始读取图像";
		QImage image = TiffHelper::ReadImage("X:\\repos\\IRayDetector\\data\\grabImg.tiff");
		//QImage image = TiffHelper::ReadImage("X:\\repos\\IRayDetector\\data\\savedImage.tiff");
		qDebug() << "读取图像结束";
		std::pair<uint16_t, uint16_t> valuesMinMax = TiffHelper::GetMinMaxValues(image);
		qDebug() << "min: " << valuesMinMax.first
			<< " max: " << valuesMinMax.second;

		qDebug() << image.format();
		label->resize(image.width(), image.height());
		qDebug() << "更新图像";
		label->setPixmap(QPixmap::fromImage(image));

		label->show();
		});

	connect(ui->pushButton_saveImage, &QPushButton::clicked, this, [this]() {
		int width = 2000;
		int height = 500;
		int value = 32768;
		// 创建16位灰度图像
		QImage image(width, height, QImage::Format_Grayscale16);

		if (image.isNull()) {
			qWarning() << "无法创建图像: 尺寸" << width << "x" << height;
			return;
		}

		// 填充所有像素为指定值
		for (int y = 0; y < height; ++y) {
			uint16_t* scanLine = reinterpret_cast<uint16_t*>(image.scanLine(y));
			for (int x = 0; x < width; ++x) {
				scanLine[x] = value;
			}
		}

		TiffHelper::SaveImage(image, "X:\\repos\\IRayDetector\\data\\savedImage.tiff");
		});

	connect(ui->pushButton_Invoke, &QPushButton::clicked, this, [this]() {
		int nCmdId = ui->lineEdit_cmdId->text().toInt();
		bool hasParam1 = ui->checkBox_nParam1->isChecked();
		bool hasParam2 = ui->checkBox_nParam2->isChecked();

		if (!hasParam1 && !hasParam2)
		{
			// 无参数调用
			int result = DET.Invoke(nCmdId);
			qDebug() << result;
		}
		else if (hasParam1 && !hasParam2)
		{
			// 单参数调用
			int nParam1 = ui->lineEdit_nParam1->text().toInt();
			int result = DET.Invoke(nCmdId, nParam1);
			qDebug() << result;
		}
		else if (hasParam1 && hasParam2)
		{
			// 双参数调用
			int nParam1 = ui->lineEdit_nParam1->text().toInt();
			int nParam2 = ui->lineEdit_nParam2->text().toInt();
			int result = DET.Invoke(nCmdId, nParam1, nParam2);
			qDebug() << result;
		}
		else
		{

		}
		});

	connect(ui->pushButton_SyncInvoke, &QPushButton::clicked, this, [this]() {
		int nCmdId = ui->lineEdit_cmdId->text().toInt();
		bool hasParam1 = ui->checkBox_nParam1->isChecked();
		bool hasParam2 = ui->checkBox_nParam2->isChecked();
		int timeout = 20000;

		if (!hasParam1 && !hasParam2)
		{
			// 无参数调用
			int result = DET.SyncInvoke(nCmdId, timeout);
			qDebug() << result;
		}
		else if (hasParam1 && !hasParam2)
		{
			// 单参数调用
			int nParam1 = ui->lineEdit_nParam1->text().toInt();
			int result = DET.SyncInvoke(nCmdId, nParam1, timeout);
			qDebug() << result;
		}
		else if (hasParam1 && hasParam2)
		{
			// 双参数调用
			int nParam1 = ui->lineEdit_nParam1->text().toInt();
			int nParam2 = ui->lineEdit_nParam2->text().toInt();
			int result = DET.SyncInvoke(nCmdId, nParam1, nParam2, timeout);
			qDebug() << result;
		}
		else
		{

		}
		});

	connect(ui->pushButton_Setnval, &QPushButton::clicked, this, [this]() {
		int nVal = ui->lineEdit_ValueToSet->text().toInt();
		int nAttrID = ui->lineEdit_nAttrIDToSet->text().toInt();
		if (!DET.SetAttr(nAttrID, nVal) ||
			!DET.WriteUserROM())
		{
			ui->lineEdit_msg->setText("写入错误！");
		}
		});

	connect(ui->pushButton_Setfval, &QPushButton::clicked, this, [this]() {
		float fVal = ui->lineEdit_ValueToSet->text().toFloat();
		int nAttrID = ui->lineEdit_nAttrIDToSet->text().toInt();
		if (!DET.SetAttr(nAttrID, fVal) ||
			!DET.WriteUserROM())
		{
			ui->lineEdit_msg->setText("写入错误！");
		}
		});

	connect(ui->pushButton_Setstrval, &QPushButton::clicked, this, [this]() {
		std::string strVal = ui->lineEdit_ValueToSet->text().toStdString();
		int nAttrID = ui->lineEdit_nAttrIDToSet->text().toInt();
		if (!DET.SetAttr(nAttrID, strVal.c_str()) ||
			!DET.WriteUserROM())
		{
			ui->lineEdit_msg->setText("写入错误！");
		}
		});

}

IRayDetectorWidget::~IRayDetectorWidget()
{
	qDebug() << "主窗口析构";
}

