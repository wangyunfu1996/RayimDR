#include "TiffHelper.h"

#include <qdebug.h>
#include <qfileinfo.h>

#include "../libtiff/include/tiff.h"
#include "../libtiff/include/tiffio.h"

TiffHelper::TiffHelper(QObject* parent)
	: QObject(parent)
{}

TiffHelper::~TiffHelper()
{}

QImage TiffHelper::ReadImage(const std::string& file_path)
{
	TIFF* tif = TIFFOpen(file_path.c_str(), "r");
	if (!tif)
	{
		qWarning() << "无法打开 TIFF 文件:" << file_path.c_str();
		return QImage();
	}

	// 使用正确的类型读取 TIFF 标签（TIFF 标准使用 uint32）
	uint32_t width = 0, height = 0;
	TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
	TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);

	uint16_t bitsPerSample = 8, samplesPerPixel = 1, photometric = 0;
	TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bitsPerSample);
	TIFFGetFieldDefaulted(tif, TIFFTAG_SAMPLESPERPIXEL, &samplesPerPixel);
	TIFFGetFieldDefaulted(tif, TIFFTAG_PHOTOMETRIC, &photometric);

	// 压缩信息
	uint16_t compression = COMPRESSION_NONE;
	TIFFGetFieldDefaulted(tif, TIFFTAG_COMPRESSION, &compression);

	// 方向
	uint16_t orientation = ORIENTATION_TOPLEFT;
	TIFFGetFieldDefaulted(tif, TIFFTAG_ORIENTATION, &orientation);

	qDebug() << "TIFF 图像信息: 尺寸=" << width << "x" << height
		<< "位深=" << bitsPerSample 
		<< "通道数=" << samplesPerPixel
		<< "压缩=" << compression;

	// 重置到第一个目录（如果之前有读取操作）
	TIFFSetDirectory(tif, 0);

	QImage image;
	if (bitsPerSample == 16 && samplesPerPixel == 1) {
		image = QImage(width, height, QImage::Format_Grayscale16);
		if (image.isNull()) {
			qWarning() << "无法创建 QImage，尺寸:" << width << "x" << height;
			TIFFClose(tif);
			return QImage();
		}

		tmsize_t scanlineSize = TIFFScanlineSize(tif);
		uint16_t* buf = (uint16_t*)_TIFFmalloc(scanlineSize);
		if (!buf) {
			qWarning() << "无法分配扫描行缓冲区";
			TIFFClose(tif);
			return QImage();
		}

		for (uint32_t row = 0; row < height; row++) {
			if (TIFFReadScanline(tif, buf, row, 0) < 0) {
				qWarning() << "读取第" << row << "行失败";
				_TIFFfree(buf);
				TIFFClose(tif);
				return QImage();
			}
			uint16_t* imageLine = reinterpret_cast<uint16_t*>(image.scanLine(row));
			memcpy(imageLine, buf, width * sizeof(uint16_t));
		}
		_TIFFfree(buf);
		
		qDebug() << "成功读取 16 位灰度 TIFF 图像:" << width << "x" << height;
	}
	else if (bitsPerSample == 8 && samplesPerPixel == 1) {
		// 支持 8 位灰度图，转换为 16 位
		image = QImage(width, height, QImage::Format_Grayscale16);
		if (image.isNull()) {
			qWarning() << "无法创建 QImage";
			TIFFClose(tif);
			return QImage();
		}

		tmsize_t scanlineSize = TIFFScanlineSize(tif);
		uint8_t* buf = (uint8_t*)_TIFFmalloc(scanlineSize);
		if (!buf) {
			qWarning() << "无法分配扫描行缓冲区";
			TIFFClose(tif);
			return QImage();
		}

		for (uint32_t row = 0; row < height; row++) {
			if (TIFFReadScanline(tif, buf, row, 0) < 0) {
				qWarning() << "读取第" << row << "行失败";
				_TIFFfree(buf);
				TIFFClose(tif);
				return QImage();
			}
			uint16_t* imageLine = reinterpret_cast<uint16_t*>(image.scanLine(row));
			// 将 8 位转换为 16 位（乘以 257 实现 0-255 到 0-65535 的映射）
			for (uint32_t col = 0; col < width; col++) {
				imageLine[col] = static_cast<uint16_t>(buf[col]) * 257;
			}
		}
		_TIFFfree(buf);
		
		qDebug() << "成功读取 8 位灰度 TIFF 图像（已转为16位）:" << width << "x" << height;
	}
	else {
		qWarning() << "不支持的 TIFF 格式: 位深=" << bitsPerSample 
			<< "通道数=" << samplesPerPixel;
	}

	TIFFClose(tif);
	return image;
}

bool TiffHelper::SaveImage(const QImage& image, const std::string& file_path)
{
	if (image.isNull()) {
		qWarning() << "图像为空";
		return false;
	}

	if (image.format() != QImage::Format_Grayscale16) {
		qWarning() << "图像格式不是 Grayscale16，当前格式:" << image.format();
		return false;
	}

	int width = image.width();
	int height = image.height();
	if (width <= 0 || height <= 0) {
		qWarning() << "无效的图像尺寸:" << width << "x" << height;
		return false;
	}

	// 打开 TIFF 文件用于写入
	TIFF* tif = TIFFOpen(file_path.c_str(), "w");
	if (!tif) {
		qWarning() << "无法创建 TIFF 文件:" << file_path.c_str();
		return false;
	}

	bool success = true;
	try {
		// 设置基本的 TIFF 标签
		TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, static_cast<uint32_t>(width));
		TIFFSetField(tif, TIFFTAG_IMAGELENGTH, static_cast<uint32_t>(height));
		TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 1);  // 单通道灰度
		TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 16);   // 16位
		TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
		TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);  // 0=黑色，最大值=白色

		// 设置样本格式为无符号整数
		TIFFSetField(tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);

		// 设置行配置
		TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize(tif, static_cast<uint32_t>(width)));

		// 准备行缓冲区
		std::vector<uint16_t> lineBuffer(width);

		// 逐行写入数据
		for (int y = 0; y < height; ++y) {
			const uint16_t* scanLine = reinterpret_cast<const uint16_t*>(image.scanLine(y));

			// 复制数据到缓冲区
			std::copy(scanLine, scanLine + width, lineBuffer.begin());

			// 写入一行数据
			if (TIFFWriteScanline(tif, lineBuffer.data(), static_cast<uint32_t>(y), 0) < 0) {
				qWarning() << "写入第" << y << "行失败";
				success = false;
				break;
			}
		}

	}
	catch (const std::exception& e) {
		qWarning() << "保存 TIFF 时发生异常:" << e.what();
		success = false;
	}

	TIFFClose(tif);

	if (!success) {
		QFile::remove(file_path.c_str());  // 删除不完整的文件
	}
	else
	{
		qDebug() << "成功保存 TIFF 文件:" << file_path.c_str()
			<< "尺寸:" << width << "x" << height
			<< "大小:" << QFileInfo(file_path.c_str()).size() << "bytes";
	}

	return success;
}

std::pair<uint16_t, uint16_t> TiffHelper::GetMinMaxValues(const QImage& image)
{
	if (image.isNull() || image.format() != QImage::Format_Grayscale16) {
		qWarning() << "无效的图像或格式不正确";
		return { 0, 0 };
	}

	uint16_t minVal = 65535;  // 16位最大值
	uint16_t maxVal = 0;

	int width = image.width();
	int height = image.height();

	for (int y = 0; y < height; ++y) {
		const uint16_t* scanLine = reinterpret_cast<const uint16_t*>(image.scanLine(y));

		for (int x = 0; x < width; ++x) {
			uint16_t pixel = scanLine[x];
			if (pixel < minVal) minVal = pixel;
			if (pixel > maxVal) maxVal = pixel;
		}
	}

	return { minVal, maxVal };
}

