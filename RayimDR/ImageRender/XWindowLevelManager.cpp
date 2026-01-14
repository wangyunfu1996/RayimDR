#include "XWindowLevelManager.h"
#include "XImageHelper.h"

XWindowLevelManager::XWindowLevelManager(QObject* parent)
	: QObject(parent), m_width(0), m_level(0)
{
}

XWindowLevelManager::~XWindowLevelManager()
{
}

void XWindowLevelManager::setWindowLevel(int width, int level)
{
	if (m_width != width || m_level != level)
	{
		m_width = width;
		m_level = level;
		emit windowLevelChanged(width, level);
	}
}

void XWindowLevelManager::calculateFromMinMax(int minValue, int maxValue)
{
	int width = 0;
	int level = 0;
	XImageHelper::calculateWLAdvanced(maxValue, minValue, width, level, 2);
	setWindowLevel(width, level);
}

void XWindowLevelManager::calculateFromROI(const QImage& image, const QRect& roiRect)
{
	if (image.isNull() || image.format() != QImage::Format_Grayscale16)
	{
		return;
	}

	int min = 65536;
	int max = 0;
	XImageHelper::calculateMaxMinValue(image, roiRect, max, min);
	calculateFromMinMax(min, max);
}

void XWindowLevelManager::reset()
{
	setWindowLevel(0, 0);
}
