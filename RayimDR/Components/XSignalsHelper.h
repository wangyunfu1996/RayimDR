#pragma once

#include <QObject>

#include "XGlobal.h"

#define xSignaHelper XSignalsHelper::Instance()
#define pXSignaHelper &XSignalsHelper::Instance()

class XSignalsHelper : public QObject
{
    Q_OBJECT

private:
    XSignalsHelper(QObject* parent = nullptr);
    ~XSignalsHelper();

public:
    static XSignalsHelper& Instance();

signals:
    void signalUpdateStatusInfo(const QString& msg);
    void signalShowErrorMessageBar(const QString& msg, int time = 3000);
    void signalShowSuccessMessageBar(const QString& msg, int time = 3000);
};
