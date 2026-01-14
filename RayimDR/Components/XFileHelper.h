#pragma once

#include <QObject>

#include <qfileinfo.h>

class XFileHelper : public QObject
{
	Q_OBJECT

public:
	XFileHelper(QObject* parent = nullptr);
	~XFileHelper();

	static QFileInfo getImageSaveFileInfo();
};

