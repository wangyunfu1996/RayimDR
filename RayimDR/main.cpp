#include <qapplication.h>
#include <qscreen.h>
#include <qdebug.h>
#include <qdatetime.h>
#include <qtimer.h>
#include <qthread.h>
#include <qmessagebox.h>

#include "ElaApplication.h"
#include "UI/MainWindow.h"
#include "IRayDetector/QtLogger.h"
#include "Components/XNetworkInfo.h"
#include "Components/XGlobal.h"

int main(int argc, char* argv[])
{
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::RoundPreferFloor);
#else
    qputenv("QT_SCALE_FACTOR", "1.5");
#endif
#endif

#ifdef _MSC_VER
    qDebug() << "MSVC Version:" << _MSC_VER;
#if _MSC_VER >= 1940
    qDebug() << "MSVC Compiler: Visual Studio 2022 version 17.10 or later";
#elif _MSC_VER >= 1930
    qDebug() << "MSVC Compiler: Visual Studio 2022 (17.0-17.9)";
#elif _MSC_VER >= 1920
    qDebug() << "MSVC Compiler: Visual Studio 2019 (16.0-16.11)";
#elif _MSC_VER >= 1910
    qDebug() << "MSVC Compiler: Visual Studio 2017 (15.0-15.9)";
#elif _MSC_VER >= 1900
    qDebug() << "MSVC Compiler: Visual Studio 2015";
#endif
    qDebug() << "MSVC Full Version:" << _MSC_FULL_VER;
#endif

    QApplication a(argc, argv);
    a.setApplicationName("RayimDR");
    a.setWindowIcon(QPixmap(":/Resource/Image/logo_32x32.ico"));
    eApp->init();

    QtLogger::initialize();

    int connectType = -1;
    for (const QHostAddress& addr : xNetworkInfo.getAllIPv4Addresses(false))
    {
        qDebug() << "addr: " << addr.toString() << " DET_HOST_IP_WIRED:" << DET_HOST_IP_WIRED << "DET_HOST_IP_WIRELESS "
                 << DET_HOST_IP_WIRELESS;
        if (addr.toString() == QString::fromStdString(DET_HOST_IP_WIRED))
        {
            qDebug() << "当前为有线连接";
            connectType = 0;
            break;
        }
        else if (addr.toString() == QString::fromStdString(DET_HOST_IP_WIRELESS))
        {
            qDebug() << "当前为无线连接";
            connectType = 1;
            break;
        }
    }

    if (connectType != 0 && connectType != 1)
    {
        QMessageBox::critical(nullptr, "网络连接错误", "未检测到与探测器的网络连接，请检查网络设置后重试！",
                              QMessageBox::Ok);
        return 0;
    }

    qDebug() << "程序运行，当前时间：" << QDateTime::currentDateTime();
    MainWindow w;
    w.setGeometry(QApplication::screens().last()->availableGeometry());
    w.showMaximized();

    return a.exec();
}