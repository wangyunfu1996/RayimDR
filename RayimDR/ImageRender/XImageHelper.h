#pragma once

#include <QObject>

#include <qimage.h>

class XImageHelper : public QObject
{
    Q_OBJECT

private:
    XImageHelper(QObject* parent = nullptr);
    ~XImageHelper();

public:
    static XImageHelper& Instance();

    static QColor generateRandomColor(int hue);
    static QPixmap generateRandomPixmap(int w, int h);
    static QPixmap generateGradientGrayPixmap(int w, int h);
    static QPixmap generateGaussianGrayPixmap(int w, int h);

    // 生成指定大小的随机高斯分布灰度图
    static QImage generateRandomGaussianGrayImage(int w, int h, QImage::Format = QImage::Format::Format_Grayscale16);
    // 计算给定灰度图的 min max
    static bool calculateMaxMinValue(const QImage& image, int& max, int& min);
    static bool calculateMaxMinValue(const QImage& image, const QRect& rect, int& max, int& min);

    // 根据灰度图的 min max 计算推荐的 width level
    static bool calculateWL(int max, int min, int& w, int& l);

    // 高级窗宽窗位计算：支持不同的算法模式
    // mode: 0=标准(全覆盖), 1=优化显示(85%范围), 2=高对比(中间50%)
    static bool calculateWLAdvanced(int max, int min, int& w, int& l, int mode = 0);
    // 从文件打开16位灰度图
    static QImage openImageU16Raw(const QString& filePath, int w, int h);
    // 将16位灰度图保存为图像文件
    static bool saveImageU16Raw(const QImage& image, const QString& filePath);
    static bool saveImagePNG(const QImage& image, const QString& filePath, int compressionLevel = 0);
    static bool saveImageJPG(const QImage& image, const QString& filePath, int quality = 100);
    // 将16位灰度图转换为8位灰度图（用于展示）
    static QImage convert16BitTo8BitLinear(const QImage& image16);
    // 弹出文件夹选择对话框，并获取其中的所有.raw文件
    static QList<QImage> openImagesInFolder(int w, int h, const QString& folderPath);
    // 对于给定的图像，返回调整窗宽窗位后的图像
    static QImage adjustWL(const QImage& image, int width, int level);

    // 关于QImage的一些测试
    static void testQImage();
    static void testOpencv(const QString& filePath, int width, int height);

signals:
    void signalOpenImageFolderProgressChanged(int progress);
};
