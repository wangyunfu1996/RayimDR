#include "XSignalsHelper.h"

XSignalsHelper::XSignalsHelper(QObject* parent) : QObject(parent) {}

XSignalsHelper::~XSignalsHelper() {}

XSignalsHelper& XSignalsHelper::Instance()
{
    static XSignalsHelper instance;
    return instance;
}