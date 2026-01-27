#pragma once

#include <QObject>

class ElaUIHepler : public QObject
{
    Q_OBJECT

public:
    ElaUIHepler(QObject* parent = nullptr);
    ~ElaUIHepler();

    static void ChangeToNormalStyle(QWidget* widget);
};
