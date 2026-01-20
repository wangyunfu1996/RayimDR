#include "IRayDetectorWidgetsApplication.h"

#include <qdebug.h>
#include <qmessagebox.h>

#include <QtConcurrent/QtConcurrent>
#include <QFuture>
#include <QFutureWatcher>

#include "../IRayDetector/IRayDetector.h"
#include "../IRayDetector/TiffHelper.h"

IRayDetectorWidgetsApplication::IRayDetectorWidgetsApplication(QWidget* parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);

	connect(ui.pushButton_connect, &QPushButton::clicked, this, [this]() {
		if (DET.Initialize() != 0)
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

	connect(ui.pushButton_disconnect, &QPushButton::clicked, this, [this]() {
		DET.DeInitialize();
		});

	connect(ui.pushButton_Abort, &QPushButton::clicked, this, [this]() {
		DET.Abort();
		});

	ui.comboBox_mode->addItem("Mode5");
	ui.comboBox_mode->addItem("Mode6");
	ui.comboBox_mode->addItem("Mode7");
	ui.comboBox_mode->addItem("Mode8");
	
	connect(ui.comboBox_mode, &QComboBox::currentTextChanged, this, [this]() {
		DET.UpdateMode(ui.comboBox_mode->currentText().toStdString());
		});

	int sw_offset{ -1 };
	int sw_gain{ -1 };
	int sw_defect{ -1 };
	DET.GetCurrentCorrectOption(sw_offset, sw_gain, sw_defect);
	ui.checkBox_offset->setChecked(sw_offset == 1);
	ui.checkBox_gain->setChecked(sw_gain == 1);
	ui.checkBox_defect->setChecked(sw_defect == 1);

	auto updateCorrectOption = [this]() {
		int sw_offset = ui.checkBox_offset->isChecked();
		int sw_gain = ui.checkBox_gain->isChecked();
		int sw_defect = ui.checkBox_defect->isChecked();
		DET.SetCorrectOption(sw_offset, sw_gain, sw_defect);
	};

	connect(ui.checkBox_offset, &QCheckBox::toggled, this, [this, updateCorrectOption](bool checked) {
		Q_UNUSED(checked);
		updateCorrectOption();
		});
	connect(ui.checkBox_gain, &QCheckBox::toggled, this, [this, updateCorrectOption](bool checked) {
		Q_UNUSED(checked);
		updateCorrectOption();
		});
	connect(ui.checkBox_defect, &QCheckBox::toggled, this, [this, updateCorrectOption](bool checked) {
		Q_UNUSED(checked);
		updateCorrectOption();
		});

	connect(ui.pushButton_nVal, &QPushButton::clicked, this, [this]() {
		ui.lineEdit_msg->clear();
		ui.lineEdit_nVal->clear();

		int nAttrID = ui.lineEdit_nAttrID->text().toInt();
		int nVal{ -1 };
		int ret = DET.GetAttr(nAttrID, nVal);
		if (0 == ret)
		{
			ui.lineEdit_nVal->setText(QString::number(nVal));
		}
		else
		{
			ui.lineEdit_msg->setText("读取错误！");
		}
		});

	connect(ui.pushButton_fVal, &QPushButton::clicked, this, [this]() {
		ui.lineEdit_msg->clear();
		ui.lineEdit_fVal->clear();

		int nAttrID = ui.lineEdit_nAttrID->text().toInt();
		float fVal{ -1.0 };
		int ret = DET.GetAttr(nAttrID, fVal);
		if (0 == ret)
		{
			ui.lineEdit_fVal->setText(QString::number(fVal));
		}
		else
		{
			ui.lineEdit_msg->setText("读取错误！");
		}
		});

	connect(ui.pushButton_strVal, &QPushButton::clicked, this, [this]() {
		ui.lineEdit_msg->clear();
		ui.lineEdit_strVal->clear();

		int nAttrID = ui.lineEdit_nAttrID->text().toInt();
		std::string strVal;
		int ret = DET.GetAttr(nAttrID, strVal);
		if (0 == ret)
		{
			ui.lineEdit_strVal->setText(QString::fromStdString(strVal));
		}
		else
		{
			ui.lineEdit_msg->setText("读取错误！");
		}
		});

	connect(ui.pushButton_clearAcq, &QPushButton::clicked, this, [this]() {
		DET.ClearAcq();
		});

	connect(ui.pushButton_StartAcq, &QPushButton::clicked, this, [this]() {
		int state{ -1 };
		DET.GetDetectorState(state);
		if (state == 2)
		{
			QMessageBox::warning(nullptr,
				"警告",
				"请等待当前采集完成，或取消当前采集任务！",
				"确定");
			return;
		}
		
		DET.StartAcq();

		});

	connect(ui.pushButton_StopAcq, &QPushButton::clicked, this, [this]() {
		DET.StopAcq();
		});

	connect(ui.pushButton_OffsetGeneration, &QPushButton::clicked, this, [this]() {
		DET.OffsetGeneration();
		});

	connect(ui.pushButton_GainGeneration, &QPushButton::clicked, this, [this]() {
		DET.GainGeneration();
		});
	connect(ui.pushButton_GainGenerationStop, &QPushButton::clicked, this, [this]() {
		DET.StopGainGeneration();
		});

	connect(ui.pushButton_readImage, &QPushButton::clicked, this, [this]() {
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

	connect(ui.pushButton_saveImage, &QPushButton::clicked, this, [this]() {
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


	if (DET.Initialize() != 0)
	{
		qDebug() << "探测器初始化失败！";
		DET.DeInitialize();
	}
	else
	{
		DET.StartQueryStatus();
	}
}

IRayDetectorWidgetsApplication::~IRayDetectorWidgetsApplication()
{
	qDebug() << "主窗口析构";
	DET.StopQueryStatus();
	DET.DeInitialize();
}

