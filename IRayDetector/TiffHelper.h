#pragma once

#include "iraydetector_global.h"

#include <QObject>
#include <QImage>

class IRAYDETECTOR_EXPORT TiffHelper : public QObject
{
	Q_OBJECT

public:
	TiffHelper(QObject* parent = nullptr);
	~TiffHelper();

	static QImage ReadImage(const std::string& file_path);
	static bool SaveImage(const QImage& image, const std::string& file_path);
	static std::pair<uint16_t, uint16_t> GetMinMaxValues(const QImage& image);
};

