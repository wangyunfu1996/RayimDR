#include "XImageHelper.h"

#include <qrandom.h>
#include <qcolor.h>
#include <qpixmap.h>
#include <qpainter.h>
#include <qdebug.h>
#include <qfile.h>
#include <qfiledialog.h>
#include <qcollator.h>
#include <qimagewriter.h>

#include <opencv2/opencv.hpp>
#include <fstream>
#include <iostream>

XImageHelper::XImageHelper(QObject* parent)
	: QObject(parent)
{}

XImageHelper::~XImageHelper()
{}

XImageHelper& XImageHelper::Instance()
{
	static XImageHelper instance;
	return instance;
}

QColor XImageHelper::generateRandomColor(int hue)
{
	return QColor::fromHsv(
		QRandomGenerator::global()->bounded(hue),		// 色调 0-359
		QRandomGenerator::global()->bounded(180, 255),  // 饱和度 150-255
		QRandomGenerator::global()->bounded(180, 255)   // 亮度 150-255
	);
}

QPixmap XImageHelper::generateRandomPixmap(int w, int h)
{
	QPixmap pixmap = QPixmap(w, h);
	QLinearGradient gradient(0, 0, pixmap.width(), pixmap.height());  // 左上到右下
	// 随机选择一个色调
	int hue = QRandomGenerator::global()->bounded(360);
	gradient.setColorAt(0, generateRandomColor(hue));
	gradient.setColorAt(1, generateRandomColor((hue + 180) % 360));
	QPainter painter(&pixmap);
	painter.fillRect(pixmap.rect(), gradient);

	return pixmap;
}

QPixmap XImageHelper::generateGradientGrayPixmap(int w, int h)
{
	QImage grayImage(w, h, QImage::Format_Grayscale8);

	// 创建从黑到白的水平渐变
	for (int y = 0; y < h; ++y)
	{
		uchar* scanLine = grayImage.scanLine(y);
		for (int x = 0; x < w; ++x)
		{
			// 计算渐变灰度值 (0-255)
			int grayValue = (x * 255) / (w - 1);
			scanLine[x] = static_cast<uchar>(grayValue);
		}
	}

	return QPixmap::fromImage(grayImage);
}

QPixmap XImageHelper::generateGaussianGrayPixmap(int w, int h)
{
	QImage image(w, h, QImage::Format_Grayscale8);

	double centerX = w / 2.0;
	double centerY = h / 2.0;
	double sigmaX = w / 4.0;  // 标准差
	double sigmaY = h / 4.0;

	for (int y = 0; y < h; ++y)
	{
		uchar* scanLine = image.scanLine(y);
		for (int x = 0; x < w; ++x)
		{
			// 计算二维高斯函数值
			double dx = (x - centerX) / sigmaX;
			double dy = (y - centerY) / sigmaY;
			double gaussianValue = exp(-0.5 * (dx * dx + dy * dy));

			// 映射到 0-255
			int grayValue = static_cast<int>(gaussianValue * 255);
			scanLine[x] = static_cast<uchar>(grayValue);
		}
	}

	return QPixmap::fromImage(image);
}

QImage XImageHelper::generateRandomGaussianGrayImage(int w, int h, QImage::Format format)
{
	if (QImage::Format::Format_Grayscale8 != format && QImage::Format::Format_Grayscale16 != format)
	{
		format = QImage::Format::Format_Grayscale8;
	}
	QImage image(w, h, format);

	std::random_device rd;
	std::mt19937 gen(rd());

	// 随机生成高斯中心
	std::uniform_real_distribution<> centerXDist(0.2 * w, 0.8 * w);
	std::uniform_real_distribution<> centerYDist(0.2 * h, 0.8 * h);
	double centerX = centerXDist(gen);
	double centerY = centerYDist(gen);

	// 随机生成标准差
	std::uniform_real_distribution<> sigmaDist(0.05 * std::min(w, h), 0.3 * std::min(w, h));
	double sigmaX = sigmaDist(gen);
	double sigmaY = sigmaDist(gen);

	// 随机生成振幅系数
	std::uniform_real_distribution<> amplitudeDist(0.5, 1.5);
	double amplitude = amplitudeDist(gen);

	// 根据目标格式选择背景灰度范围
	int bgGray;
	if (format == QImage::Format_Grayscale16) {
		// 16位背景灰度范围 (0-65535)
		std::uniform_int_distribution<> bgDist(0, 1000); // 较低背景值
		bgGray = bgDist(gen);
	}
	else {
		// 8位背景灰度范围 (0-255)
		std::uniform_int_distribution<> bgDist(0, 50);
		bgGray = bgDist(gen);
	}

	// 填充背景 - 根据格式选择不同的填充方式
	if (format == QImage::Format_Grayscale16) {
		// 16位图像填充
		for (int y = 0; y < h; ++y) {
			ushort* scanLine = reinterpret_cast<ushort*>(image.scanLine(y));
			for (int x = 0; x < w; ++x) {
				scanLine[x] = static_cast<ushort>(bgGray);
			}
		}
	}
	else {
		// 8位图像填充
		for (int y = 0; y < h; ++y) {
			uchar* scanLine = image.scanLine(y);
			for (int x = 0; x < w; ++x) {
				scanLine[x] = static_cast<uchar>(bgGray);
			}
		}
	}

	// 添加高斯分布
	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			// 计算高斯函数值
			double dx = (x - centerX) / sigmaX;
			double dy = (y - centerY) / sigmaY;
			double gaussianValue = exp(-0.5 * (dx * dx + dy * dy));

			// 应用随机振幅
			gaussianValue *= amplitude;

			// 根据目标格式计算最终的灰度值并混合
			if (format == QImage::Format_Grayscale16) {
				// 16位图像：计算16位灰度值 (0-65535)
				int grayValue = static_cast<int>(gaussianValue * 65535);
				grayValue = std::min(65535, std::max(0, grayValue));

				// 与背景混合（取最大值）
				ushort* pixelPtr = reinterpret_cast<ushort*>(image.scanLine(y)) + x;
				*pixelPtr = static_cast<ushort>(std::max(static_cast<int>(*pixelPtr), grayValue));
			}
			else {
				// 8位图像：计算8位灰度值 (0-255)
				int grayValue = static_cast<int>(gaussianValue * 255);
				grayValue = std::min(255, std::max(0, grayValue));

				// 与背景混合（取最大值）
				uchar* pixelPtr = image.scanLine(y) + x;
				*pixelPtr = static_cast<uchar>(std::max(static_cast<int>(*pixelPtr), grayValue));
			}
		}
	}

	qDebug() << image.format();
	return image;
}

bool XImageHelper::calculateMaxMinValue(const QImage& image, int& max, int& min)
{
	// 计算灰度图的最小最大值
	// 检查图像格式
	if (image.format() == QImage::Format_Grayscale8) {
		// 8位灰度图
		int minValue = 255;  // 8位灰度图最大值
		int maxValue = 0;    // 8位灰度图最小值
		for (int y = 0; y < image.height(); ++y) {
			const uchar* scanLine = image.constScanLine(y);
			for (int x = 0; x < image.width(); ++x) {
				int gray = scanLine[x];
				if (gray < minValue) minValue = gray;
				if (gray > maxValue) maxValue = gray;
			}
		}

		max = maxValue;
		min = minValue;
		return true;
	}
	else if (image.format() == QImage::Format_Grayscale16) {
		// 16位灰度图
		int minValue = 65535;	// 16位灰度图最大值
		int maxValue = 0;		// 16位灰度图最小值
		for (int y = 0; y < image.height(); ++y) {
			const ushort* scanLine = reinterpret_cast<const ushort*>(image.constScanLine(y));
			for (int x = 0; x < image.width(); ++x) {
				int gray = scanLine[x];
				if (gray < minValue) minValue = gray;
				if (gray > maxValue) maxValue = gray;
			}
		}

		max = maxValue;
		min = minValue;
		return true;
	}
	else
	{
		qDebug() << "错误的图像格式：" << image.format();
		return false;
	}
}

bool XImageHelper::calculateMaxMinValue(const QImage& image, const QRect& rect, int& max, int& min)
{
	// 参数检查和区域裁剪
	if (image.isNull() || image.format() != QImage::Format_Grayscale16 || !rect.isValid()) {
		min = max = 0;
		return false;
	}

	QRect validRegion = rect.intersected(image.rect());
	if (validRegion.isEmpty()) {
		min = max = 0;
		return false;
	}

	// 初始化最大最小值
	min = 65536; // 最大值初始化
	max = 0;   // 最小值初始化

	// 遍历指定区域内的每一个像素
	for (int y = validRegion.top(); y <= validRegion.bottom(); ++y) {
		for (int x = validRegion.left(); x <= validRegion.right(); ++x) {
			// 获取该行数据的常量指针，并重新解释为 ushort 指针
			const uchar* scanLine = image.constScanLine(y); // 获取第y行数据的起始地址
			const ushort* pixelData = reinterpret_cast<const ushort*>(scanLine); // 解释为16位数据

			// 读取该像素的16位灰度值
			ushort grayValue16 = pixelData[x]; // 范围是 0 到 65535

			// 更新最大最小值
			if (grayValue16 < min) {
				min = grayValue16;
			}
			if (grayValue16 > max) {
				max = grayValue16;
			}
		}
	}

	return true;
}

bool XImageHelper::calculateWL(int max, int min, int& w, int& l)
{
	if (max < min)
	{
		return false;
	}

	w = max - min;
	l = (max + min) / 2;
	return false;
}

QImage XImageHelper::openImageU16Raw(const QString& filePath, int w, int h)
{
	if (filePath.isEmpty() || w <= 0 || h <= 0) {
		qWarning() << "传入参数错误：" << filePath << w << h;
		return QImage();
	}

	QFile file(filePath);
	if (!file.open(QIODevice::ReadOnly)) {
		qWarning() << "Cannot open file:" << filePath << file.errorString();
		return QImage();
	}

	// 计算预期的文件大小（16位 = 2字节/像素）
	qint64 expectedSize = static_cast<qint64>(w) * h * 2;
	if (file.size() != expectedSize) {
		qWarning() << "File size mismatch. Expected:" << expectedSize << "Actual:" << file.size();
		file.close();
		return QImage();
	}

	// 读取所有数据
	QByteArray data = file.readAll();
	file.close();

	if (data.size() != expectedSize) {
		qWarning() << "Read data size mismatch";
		return QImage();
	}

	// 创建16位灰度图像（需要Qt 5.13+）
	QImage image(w, h, QImage::Format_Grayscale16);

	// 将原始数据复制到图像
	const uchar* srcData = reinterpret_cast<const uchar*>(data.constData());
	for (int y = 0; y < h; ++y) {
		ushort* destLine = reinterpret_cast<ushort*>(image.scanLine(y));
		const ushort* srcLine = reinterpret_cast<const ushort*>(srcData + y * w * 2);
		std::copy(srcLine, srcLine + w, destLine);
	}

	return image;
}

bool XImageHelper::saveImageU16Raw(const QImage& image, const QString& filePath)
{
	// 参数检查
	if (filePath.isEmpty()) {
		qWarning() << "文件路径为空";
		return false;
	}

	if (image.isNull()) {
		qWarning() << "图像为空";
		return false;
	}

	// 检查图像格式
	if (image.format() != QImage::Format_Grayscale16) {
		qWarning() << "图像格式必须是Format_Grayscale16，当前格式:" << image.format();

		// 可选：自动转换为16位灰度格式
		QImage converted = image.convertToFormat(QImage::Format_Grayscale16);
		if (converted.isNull()) {
			qWarning() << "无法转换为16位灰度格式";
			return false;
		}
		return saveImageU16Raw(converted, filePath);
	}

	// 获取图像尺寸
	int w = image.width();
	int h = image.height();

	if (w <= 0 || h <= 0) {
		qWarning() << "图像尺寸无效:" << w << "x" << h;
		return false;
	}

	// 创建文件
	QFile file(filePath);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		qWarning() << "无法创建文件:" << filePath << file.errorString();
		return false;
	}

	// 提取图像数据
	try {
		// 方法1：逐行写入（更安全）
		for (int y = 0; y < h; ++y) {
			const ushort* lineData = reinterpret_cast<const ushort*>(image.constScanLine(y));
			if (lineData == nullptr) {
				qWarning() << "无法获取第" << y << "行的扫描线数据";
				file.close();
				return false;
			}

			// 写入一行数据
			qint64 bytesWritten = file.write(
				reinterpret_cast<const char*>(lineData),
				w * sizeof(ushort)
			);

			if (bytesWritten != w * sizeof(ushort)) {
				qWarning() << "写入第" << y << "行时出错，预期写入:"
					<< w * sizeof(ushort) << "字节，实际写入:" << bytesWritten;
				file.close();
				return false;
			}
		}

		file.close();
		qDebug() << "图像保存成功:" << filePath << "尺寸:" << w << "x" << h;
		return true;

	}
	catch (const std::exception& e) {
		qWarning() << "保存过程中发生异常:" << e.what();
		file.close();
		return false;
	}
}

/**
 * @brief 保存灰度图像为PNG格式（无损压缩，支持透明度）
 * @param image 要保存的图像（应为灰度图）
 * @param filePath 文件保存路径
 * @param compressionLevel 压缩级别 (0=无压缩, 9=最高压缩)
 * @return 保存成功返回true，否则返回false
 */
bool XImageHelper::saveImagePNG(const QImage& image, const QString& filePath, int compressionLevel)
{
	// 参数验证
	if (image.isNull()) {
		qWarning() << "图像为空";
		return false;
	}

	if (filePath.isEmpty()) {
		qWarning() << "文件路径为空";
		return false;
	}

	// 确保目录存在
	QFileInfo fileInfo(filePath);
	QDir dir = fileInfo.dir();
	if (!dir.exists()) {
		if (!dir.mkpath(".")) {
			qWarning() << "无法创建目录" << dir.path();
			return false;
		}
	}

	if (image.format() != QImage::Format_Grayscale8)
	{
		// 转换为灰度格式（确保是灰度图）
		QImage grayImage = convert16BitTo8BitLinear(image);
		if (grayImage.isNull()) {
			qWarning() << "转换为灰度图失败";
			return false;
		}
	}

	// 保存PNG
	QImageWriter writer(filePath);
	writer.setFormat("png");

	// 设置PNG特定参数
	writer.setQuality(100); // PNG是无损的，但Qt仍需要这个参数
	writer.setCompression(compressionLevel); // 0-9, 0=无压缩, 9=最高压缩

	bool success = writer.write(image);

	if (success) {
		qint64 fileSize = QFileInfo(filePath).size();
		qDebug() << "PNG保存成功:"
			<< filePath
			<< "尺寸:" << image.size()
			<< "格式:" << image.format()
			<< "大小:" << fileSize
			<< "压缩级别:" << compressionLevel;
	}
	else {
		qWarning() << "saveImagePNG: 保存失败" << filePath
			<< "错误:" << writer.errorString();
	}

	return success;
}

/**
 * @brief 保存灰度图像为JPG格式（有损压缩）
 * @param image 要保存的图像（应为灰度图）
 * @param filePath 文件保存路径
 * @param quality 质量参数 (0-100, 100为最佳质量)
 * @return 保存成功返回true，否则返回false
 */
bool XImageHelper::saveImageJPG(const QImage& image, const QString& filePath, int quality)
{
	// 参数验证
	if (image.isNull()) {
		qWarning() << "saveImageJPG: 图像为空";
		return false;
	}

	if (filePath.isEmpty()) {
		qWarning() << "saveImageJPG: 文件路径为空";
		return false;
	}

	// 确保目录存在
	QFileInfo fileInfo(filePath);
	QDir dir = fileInfo.dir();
	if (!dir.exists()) {
		if (!dir.mkpath(".")) {
			qWarning() << "saveImageJPG: 无法创建目录" << dir.path();
			return false;
		}
	}

	// 转换为8位灰度图（JPG只支持8位灰度）
	QImage grayImage = convert16BitTo8BitLinear(image);
	if (grayImage.isNull()) {
		qWarning() << "saveImageJPG: 转换为8位灰度图失败";
		return false;
	}

	// 保存JPG
	QImageWriter writer(filePath);
	writer.setFormat("jpeg");

	// 设置JPG参数
	writer.setQuality(quality);

	// 设置优化编码
	writer.setOptimizedWrite(true);

	// JPG不支持16位，所以确保是8位
	if (grayImage.format() != QImage::Format_Grayscale8) {
		grayImage = grayImage.convertToFormat(QImage::Format_Grayscale8);
	}

	bool success = writer.write(grayImage);

	if (success) {
		qint64 fileSize = QFileInfo(filePath).size();
		qDebug() << "JPG保存成功:"
			<< filePath
			<< "尺寸:" << grayImage.size()
			<< "格式: 8位灰度"
			<< "大小:" << fileSize
			<< "质量:" << quality;
	}
	else {
		qWarning() << "saveImageJPG: 保存失败" << filePath
			<< "错误:" << writer.errorString();
	}

	return success;
}

QList<QImage> XImageHelper::openImagesInFolder(int w, int h, const QString& folderPath)
{
	if (folderPath.isEmpty()) {
		qDebug() << "未选择文件夹";
		return QList<QImage>();
	}

	// 创建QDir对象指向该文件夹
	QDir dir(folderPath);

	// 设置过滤器：只获取文件，并排除 "." 和 ".."
	dir.setFilter(QDir::Files | QDir::NoDotAndDotDot);

	// 获取文件夹内所有文件的名称列表
	QStringList fileNames = dir.entryList();
	QCollator collator;
	collator.setNumericMode(true);
	std::sort(fileNames.begin(), fileNames.end(), collator);

	qDebug() << "文件夹:" << folderPath;
	qDebug() << "包含文件数量:" << fileNames.size();

	QList<QImage> imageList;
	int idx = 0;
	// 遍历并输出文件名
	for (const QString& fileName : fileNames) {
		if (fileName.endsWith(".raw"))
		{
			QImage image = openImageU16Raw(dir.absoluteFilePath(fileName), w, h);
			imageList.append(image);
			idx++;
			//qDebug() << (idx + 1.0) / fileNames.size() * 100;
			emit XImageHelper::Instance().signalOpenImageFolderProgressChanged((idx + 1.0) / fileNames.size() * 100);
		}
		else
		{
			qDebug() << "无效的文件：" << dir.absoluteFilePath(fileName);
		}
	}

	return imageList;
}

QImage XImageHelper::adjustWL(const QImage& image, int width, int level)
{
	// 获取图像
	if (image.isNull() || image.format() != QImage::Format_Grayscale16) {
		return QImage();
	}

	int w = image.width();
	int h = image.height();

	// 创建8位输出图像用于显示
	QImage dest(w, h, QImage::Format_Grayscale8);
	dest.fill(0);

	// 计算窗口边界
	int windowMin = level - width / 2;
	int windowMax = level + width / 2;
	float windowRange = windowMax - windowMin;

	// 避免除零错误
	if (windowRange <= 0) {
		windowRange = 1;
	}

	// 遍历所有像素并应用线性映射
	for (int y = 0; y < h; ++y) {
		const uint16_t* srcLine = reinterpret_cast<const uint16_t*>(image.scanLine(y));
		uchar* dstLine = dest.scanLine(y);

		for (int x = 0; x < w; ++x) {
			uint16_t pixelValue = srcLine[x];

			// 线性映射：将 [windowMin, windowMax] 映射到 [0, 255]
			float normalized = 255.0f * (pixelValue - windowMin) / windowRange;

			// 饱和处理：小于窗口最小值 -> 0，大于窗口最大值 -> 255
			if (normalized < 0) normalized = 0;
			if (normalized > 255) normalized = 255;

			dstLine[x] = static_cast<uchar>(normalized);
		}
	}

	return dest;
}

void XImageHelper::testQImage()
{
	QImage i2;
	{
		// 测试QImage的引用计数和写时复制
		QImage i1(2048, 2048, QImage::Format_Grayscale16);
		i1.fill(63356);
		qDebug() << i1.constBits();
		i2 = i1;
		i1.setPixel(1024, 1024, 0);
		qDebug() << i1.constBits();
		qDebug() << i2.constBits();
	}
	qDebug() << i2.constBits();

	// 创建测试图像
	QImage original(100, 100, QImage::Format_ARGB32);
	original.fill(QColor(255, 0, 0, 255));  // 红色

	// 测试1：默认赋值
	QImage copy1 = original;

	bool isShallow1 = (original.constBits() == copy1.constBits());
	qDebug() << "测试1 - 默认赋值后地址相同:" << isShallow1;

	isShallow1 = (original.bits() == copy1.bits());
	qDebug() << "测试1 - 默认赋值后地址相同:" << isShallow1;

	// 测试2：修改copy1触发写时复制
	copy1.setPixel(10, 10, qRgba(0, 255, 0, 255));
	bool isShallowAfterModify = (original.bits() == copy1.bits());
	qDebug() << "测试2 - 修改后地址相同:" << isShallowAfterModify;
	qDebug() << "原始像素(10,10):" << QColor(original.pixel(10, 10));
	qDebug() << "copy1像素(10,10):" << QColor(copy1.pixel(10, 10));

	// 测试3：使用copy()方法
	QImage copy2 = original.copy();
	bool isDeepCopy = (original.bits() == copy2.bits());
	qDebug() << "测试3 - copy()方法地址相同:" << isDeepCopy;

	// 测试4：修改copy2不影响原始图像
	copy2.setPixel(20, 20, qRgba(0, 0, 255, 255));
	qDebug() << "测试4 - 修改深拷贝后:";
	qDebug() << "原始像素(20,20):" << QColor(original.pixel(20, 20));
	qDebug() << "copy2像素(20,20):" << QColor(copy2.pixel(20, 20));

	// 测试5：isDetached()方法检测是否分离
	QImage copy3 = original;
	qDebug() << "测试5 - 创建时isDetached:" << copy3.isDetached();  // false
	copy3.setPixel(30, 30, qRgba(255, 255, 0, 255));
	qDebug() << "修改后isDetached:" << copy3.isDetached();  // true
}

void XImageHelper::testOpencv(const QString& filePath, int width, int height)
{
	auto readRaw16Bit = [](const QString& filePath, int width, int height) {
		// 使用QFile读取文件到内存
		QFile file(filePath);
		if (!file.open(QIODevice::ReadOnly)) {
			qDebug() << "无法打开文件:" << filePath;
			qDebug() << "错误:" << file.errorString();
			return cv::Mat();
		}

		QByteArray imageData = file.readAll();
		file.close();

		if (imageData.isEmpty()) {
			qDebug() << "文件为空或读取失败:" << filePath;
			return cv::Mat();
		}

		if (imageData.size() != width * height * 2)
		{
			qDebug() << "文件为空或读取失败，图像大小不匹配！";
			return cv::Mat();
		}

		// 创建16位Mat
		cv::Mat image(height, width, CV_16UC1);

		// 将数据复制到Mat中
		// 注意：这里假设数据是小端字节序
		memcpy(image.data, imageData.constData(), width * height * 2);

		return image;
	};

	auto data = readRaw16Bit(filePath, width, height);

	auto display16BitImage = [](const cv::Mat& data) {
		if (data.empty()) return;

		cv::Mat displayImage;

		// 找到最小值和最大值
		double minVal, maxVal;
		cv::minMaxLoc(data, &minVal, &maxVal);

		if (maxVal > minVal) {
			// 线性拉伸到0-255
			data.convertTo(displayImage, CV_8UC1,
				255.0 / (maxVal - minVal),
				-255.0 * minVal / (maxVal - minVal));
		}
		else {
			// 如果所有值相同
			data.convertTo(displayImage, CV_8UC1, 255.0 / maxVal);
		}

		cv::imshow("16-bit RAW Image (8-bit display)", displayImage);
		cv::waitKey(0);
	};

	display16BitImage(data);
}

QImage XImageHelper::convert16BitTo8BitLinear(const QImage& image16)
{
	if (image16.format() == QImage::Format_Grayscale8)
	{
		return image16;
	}

	int w = image16.width();
	int h = image16.height();
	QImage image8(w, h, QImage::Format_Grayscale8);

	// 找到16位图像的实际最小值和最大值
	ushort minVal = 65535, maxVal = 0;
	for (int y = 0; y < h; ++y) {
		const ushort* line16 = (const ushort*)image16.constScanLine(y);
		for (int x = 0; x < w; ++x) {
			ushort val = line16[x];
			if (val < minVal) minVal = val;
			if (val > maxVal) maxVal = val;
		}
	}
	double range = maxVal - minVal;
	if (range < 1.0) range = 1.0; // 避免除以零

	// 线性映射并填充8位图像
	for (int y = 0; y < h; ++y) {
		const ushort* line16 = (const ushort*)image16.constScanLine(y);
		uchar* line8 = image8.scanLine(y);
		for (int x = 0; x < w; ++x) {
			double normalized = (line16[x] - minVal) / range; // 归一化到0~1
			line8[x] = static_cast<uchar>(normalized * 255.0);
		}
	}
	return image8;
}

