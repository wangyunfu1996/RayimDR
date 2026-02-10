#include "IniReader.h"

#include <QFile>
#include <QTextStream>
#include <QDebug>

IniReader::IniReader(QObject* parent) : QObject(parent), m_loaded(false) {}

IniReader::IniReader(const QString& filePath, QObject* parent) : QObject(parent), m_loaded(false)
{
    load(filePath);
}

IniReader::~IniReader() {}

bool IniReader::load(const QString& filePath)
{
    clear();
    m_filePath = filePath;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        m_lastError = QString("Failed to open file: %1").arg(filePath);
        qWarning() << m_lastError;
        return false;
    }

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Encoding::Utf8);

    QString currentSection;
    int lineNumber = 0;

    while (!in.atEnd())
    {
        QString line = in.readLine().trimmed();
        lineNumber++;

        if (line.isEmpty() || line.startsWith(';') || line.startsWith('#'))
        {
            continue;
        }

        if (line.startsWith('[') && line.endsWith(']'))
        {
            currentSection = line.mid(1, line.length() - 2).trimmed();
            if (!m_data.contains(currentSection))
            {
                m_data[currentSection] = QMap<QString, QString>();
                m_sectionOrder.append(currentSection);
                m_keyOrder[currentSection] = QStringList();
            }
            continue;
        }

        int equalPos = line.indexOf('=');
        if (equalPos > 0)
        {
            if (currentSection.isEmpty())
            {
                m_lastError = QString("Key-value pair found before any section at line %1").arg(lineNumber);
                qWarning() << m_lastError;
                continue;
            }

            QString key = line.left(equalPos).trimmed();
            QString value = parseValue(line, equalPos);

            if (!m_data[currentSection].contains(key))
            {
                m_keyOrder[currentSection].append(key);
            }
            m_data[currentSection][key] = value;
        }
    }

    file.close();
    m_loaded = true;
    m_lastError.clear();

    return true;
}

bool IniReader::isLoaded() const
{
    return m_loaded;
}

void IniReader::clear()
{
    m_data.clear();
    m_sectionOrder.clear();
    m_keyOrder.clear();
    m_filePath.clear();
    m_lastError.clear();
    m_loaded = false;
}

QString IniReader::getString(const QString& section, const QString& key, const QString& defaultValue) const
{
    if (!m_data.contains(section))
    {
        return defaultValue;
    }

    const QMap<QString, QString>& sectionData = m_data[section];
    if (!sectionData.contains(key))
    {
        return defaultValue;
    }

    return sectionData[key];
}

int IniReader::getInt(const QString& section, const QString& key, int defaultValue) const
{
    QString value = getString(section, key);
    if (value.isEmpty())
    {
        return defaultValue;
    }

    bool ok;
    int result = value.toInt(&ok);
    return ok ? result : defaultValue;
}

double IniReader::getDouble(const QString& section, const QString& key, double defaultValue) const
{
    QString value = getString(section, key);
    if (value.isEmpty())
    {
        return defaultValue;
    }

    bool ok;
    double result = value.toDouble(&ok);
    return ok ? result : defaultValue;
}

bool IniReader::getBool(const QString& section, const QString& key, bool defaultValue) const
{
    QString value = getString(section, key).toLower();
    if (value.isEmpty())
    {
        return defaultValue;
    }

    if (value == "true" || value == "1" || value == "yes" || value == "on")
    {
        return true;
    }
    else if (value == "false" || value == "0" || value == "no" || value == "off")
    {
        return false;
    }

    return defaultValue;
}

bool IniReader::hasSection(const QString& section) const
{
    return m_data.contains(section);
}

bool IniReader::hasKey(const QString& section, const QString& key) const
{
    if (!m_data.contains(section))
    {
        return false;
    }

    return m_data[section].contains(key);
}

QStringList IniReader::getSections() const
{
    return m_sectionOrder;
}

QStringList IniReader::getKeys(const QString& section) const
{
    if (!m_data.contains(section))
    {
        return QStringList();
    }

    return m_keyOrder[section];
}

QString IniReader::getLastError() const
{
    return m_lastError;
}

QString IniReader::parseValue(const QString& line, int equalPos) const
{
    QString value = line.mid(equalPos + 1).trimmed();
    value = trimComment(value);

    if ((value.startsWith('"') && value.endsWith('"')) || (value.startsWith('\'') && value.endsWith('\'')))
    {
        value = value.mid(1, value.length() - 2);
    }

    return value;
}

QString IniReader::trimComment(const QString& line) const
{
    bool inQuotes = false;
    QChar quoteChar;

    for (int i = 0; i < line.length(); ++i)
    {
        QChar ch = line[i];

        if ((ch == '"' || ch == '\'') && (i == 0 || line[i - 1] != '\\'))
        {
            if (!inQuotes)
            {
                inQuotes = true;
                quoteChar = ch;
            }
            else if (ch == quoteChar)
            {
                inQuotes = false;
            }
        }

        if (!inQuotes && (ch == ';' || ch == '#'))
        {
            return line.left(i).trimmed();
        }
    }

    return line.trimmed();
}

QString IniReader::makeKey(const QString& section, const QString& key) const
{
    return section + "::" + key;
}

void IniReader::setString(const QString& section, const QString& key, const QString& value)
{
    if (section.isEmpty() || key.isEmpty())
    {
        m_lastError = "Section or key cannot be empty";
        qWarning() << m_lastError;
        return;
    }

    bool isNewSection = !m_data.contains(section);
    bool isNewKey = !m_data[section].contains(key);

    if (isNewSection)
    {
        m_data[section] = QMap<QString, QString>();
        m_sectionOrder.append(section);
        m_keyOrder[section] = QStringList();
    }

    if (isNewKey)
    {
        m_keyOrder[section].append(key);
    }

    m_data[section][key] = value;
}

void IniReader::setInt(const QString& section, const QString& key, int value)
{
    setString(section, key, QString::number(value));
}

void IniReader::setDouble(const QString& section, const QString& key, double value)
{
    setString(section, key, QString::number(value));
}

void IniReader::setBool(const QString& section, const QString& key, bool value)
{
    setString(section, key, value ? "true" : "false");
}

bool IniReader::removeKey(const QString& section, const QString& key)
{
    if (!m_data.contains(section))
    {
        m_lastError = QString("Section '%1' does not exist").arg(section);
        qWarning() << m_lastError;
        return false;
    }

    if (!m_data[section].contains(key))
    {
        m_lastError = QString("Key '%1' does not exist in section '%2'").arg(key, section);
        qWarning() << m_lastError;
        return false;
    }

    m_data[section].remove(key);
    m_keyOrder[section].removeAll(key);

    // Remove section if it becomes empty
    if (m_data[section].isEmpty())
    {
        m_data.remove(section);
        m_sectionOrder.removeAll(section);
        m_keyOrder.remove(section);
    }

    return true;
}

bool IniReader::removeSection(const QString& section)
{
    if (!m_data.contains(section))
    {
        m_lastError = QString("Section '%1' does not exist").arg(section);
        qWarning() << m_lastError;
        return false;
    }

    m_data.remove(section);
    m_sectionOrder.removeAll(section);
    m_keyOrder.remove(section);
    return true;
}

bool IniReader::save()
{
    if (m_filePath.isEmpty())
    {
        m_lastError = "No file path specified. Use saveAs() instead.";
        qWarning() << m_lastError;
        return false;
    }

    return saveAs(m_filePath);
}

bool IniReader::saveAs(const QString& filePath)
{
    if (filePath.isEmpty())
    {
        m_lastError = "File path cannot be empty";
        qWarning() << m_lastError;
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        m_lastError = QString("Failed to open file for writing: %1").arg(filePath);
        qWarning() << m_lastError;
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Encoding::Utf8);

    // Use recorded section order instead of QMap keys
    for (int i = 0; i < m_sectionOrder.size(); ++i)
    {
        const QString& section = m_sectionOrder[i];

        // Skip if section was removed
        if (!m_data.contains(section))
        {
            continue;
        }

        // Write section header
        out << "[" << section << "]" << Qt::endl;

        // Write key-value pairs in recorded order
        const QMap<QString, QString>& sectionData = m_data[section];
        const QStringList& keys = m_keyOrder[section];
        for (const QString& key : keys)
        {
            // Skip if key was removed
            if (!sectionData.contains(key))
            {
                continue;
            }

            const QString& value = sectionData[key];

            // Add quotes if value contains special characters
            if (value.contains(' ') || value.contains(';') || value.contains('#') || value.contains('='))
            {
                out << key << "=\"" << value << "\"" << Qt::endl;
            }
            else
            {
                out << key << "=" << value << Qt::endl;
            }
        }

        // Add blank line between sections (except after the last section)
        if (i < m_sectionOrder.size() - 1)
        {
            out << Qt::endl;
        }
    }

    file.close();

    // Update file path if successfully saved
    m_filePath = filePath;
    m_lastError.clear();

    return true;
}
