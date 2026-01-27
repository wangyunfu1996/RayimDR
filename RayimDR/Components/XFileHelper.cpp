#include "XFileHelper.h"

#include <qfiledialog.h>
#include <qstandardpaths.h>

XFileHelper::XFileHelper(QObject* parent) : QObject(parent) {}

XFileHelper::~XFileHelper() {}

QFileInfo XFileHelper::getImageSaveFileInfo()
{
    static QDir lastSelectedDir;

    // 如果上次目录无效，使用默认目录
    if (lastSelectedDir.path().isEmpty() || !lastSelectedDir.exists())
    {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        lastSelectedDir = QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
#else
        lastSelectedDir = QDir(QStandardPaths::writableLocation(QStandardPaths::DataLocation));
#endif
    }

    // 获取保存文件的路径和格式
    QString fileName = QFileDialog::getSaveFileName(
        nullptr, QObject::tr("保存图像文件"),
        lastSelectedDir.absoluteFilePath("Image"),  // 默认文件名
        QObject::tr("RAW文件 (*.raw);;TIFF文件 (*.tif);;PNG文件 (*.png);;JPEG文件 (*.jpg;*.jpeg);;所有文件 (*)"));

    // 检查是否取消操作
    if (fileName.isEmpty())
    {
        return QFileInfo();
    }

    // 获取文件信息并更新最后选择的目录
    QFileInfo fileInfo(fileName);
    lastSelectedDir = fileInfo.absoluteDir();

    return fileInfo;
}
