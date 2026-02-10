#include "XGlobal.h"

#include <qapplication.h>

#include "Components/IniReader.h"

XGlobal::XGlobal() {}

XGlobal::~XGlobal()
{
    if (m_IniReader)
    {
        m_IniReader->save();
        delete m_IniReader;
        m_IniReader = nullptr;
    }
}

bool XGlobal::init()
{
    // 读取配置文件
    QString configPath = QApplication::applicationDirPath() + "/config.ini";
    m_IniReader = new IniReader(configPath);

    if (!m_IniReader->isLoaded())
    {
        qDebug() << "配置文件错误", "配置文件加载失败，请检查配置文件后重试！";
        return false;
    }

    for (auto section : m_IniReader->getSections())
    {
        qDebug() << "Section:" << section;
        for (auto key : m_IniReader->getKeys(section))
        {
            qDebug() << "  Key:" << key << "Value:" << m_IniReader->getString(section, key);
        }
    }

    return true;
}

QString XGlobal::getString(const QString& section, const QString& key, const QString& defaultValue) const
{
    return m_IniReader->getString(section, key, defaultValue);
}

int XGlobal::getInt(const QString& section, const QString& key, int defaultValue) const
{
    return m_IniReader->getInt(section, key, defaultValue);
}

double XGlobal::getDouble(const QString& section, const QString& key, double defaultValue) const
{
    return m_IniReader->getDouble(section, key, defaultValue);
}

bool XGlobal::getBool(const QString& section, const QString& key, bool defaultValue) const
{
    return m_IniReader->getBool(section, key, defaultValue);
}

void XGlobal::setString(const QString& section, const QString& key, const QString& value)
{
    return m_IniReader->setString(section, key, value);
}

void XGlobal::setInt(const QString& section, const QString& key, int value)
{
    return m_IniReader->setInt(section, key, value);
}

void XGlobal::setDouble(const QString& section, const QString& key, double value)
{
    return m_IniReader->setDouble(section, key, value);
}

void XGlobal::setBool(const QString& section, const QString& key, bool value)
{
    return m_IniReader->setBool(section, key, value);
}

bool XGlobal::save()
{
    return m_IniReader->save();
}
