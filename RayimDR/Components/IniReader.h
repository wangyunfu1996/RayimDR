#pragma once

#include <QObject>
#include <QString>
#include <QMap>
#include <QVariant>

class IniReader : public QObject
{
    Q_OBJECT

public:
    explicit IniReader(QObject* parent = nullptr);
    explicit IniReader(const QString& filePath, QObject* parent = nullptr);
    ~IniReader();

    bool load(const QString& filePath);
    bool isLoaded() const;
    void clear();

    QString getString(const QString& section, const QString& key, const QString& defaultValue = QString()) const;
    int getInt(const QString& section, const QString& key, int defaultValue = 0) const;
    double getDouble(const QString& section, const QString& key, double defaultValue = 0.0) const;
    bool getBool(const QString& section, const QString& key, bool defaultValue = false) const;

    bool hasSection(const QString& section) const;
    bool hasKey(const QString& section, const QString& key) const;
    QStringList getSections() const;
    QStringList getKeys(const QString& section) const;

    QString getLastError() const;

    // Write operations
    void setString(const QString& section, const QString& key, const QString& value);
    void setInt(const QString& section, const QString& key, int value);
    void setDouble(const QString& section, const QString& key, double value);
    void setBool(const QString& section, const QString& key, bool value);

    bool removeKey(const QString& section, const QString& key);
    bool removeSection(const QString& section);

    bool save();
    bool saveAs(const QString& filePath);

private:
    QString parseValue(const QString& line, int equalPos) const;
    QString trimComment(const QString& line) const;
    QString makeKey(const QString& section, const QString& key) const;

private:
    QMap<QString, QMap<QString, QString>> m_data;
    QStringList m_sectionOrder;
    QMap<QString, QStringList> m_keyOrder;
    QString m_filePath;
    QString m_lastError;
    bool m_loaded;
};
