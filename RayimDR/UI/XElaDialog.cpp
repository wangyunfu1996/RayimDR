#include "XElaDialog.h"

#include <qlayout.h>
#include <qscreen.h>
#include <qguiapplication.h>
#include <qcursor.h>
#include <qdebug.h>

#include "ElaPushButton.h"
#include "ElaText.h"

XElaDialog::XElaDialog(QString msg, XElaDialogType type, QWidget* parent) : ElaDialog(parent)
{
    this->setWindowModality(Qt::ApplicationModal);
    this->setWindowButtonFlag(ElaAppBarType::NoneButtonHint);
    this->setAppBarHeight(0);

    this->resize(300, 100);

    auto vLayout = new QVBoxLayout;
    vLayout->setContentsMargins(12, 9, 12, 9);
    this->setLayout(vLayout);

    ElaText* titleText = new ElaText(this);
    titleText->setTextStyle(ElaTextType::Title);
    if (XElaDialogType::INFO == type)
    {
        titleText->setText("提示");
    }
    else if (XElaDialogType::ERR == type)
    {
        titleText->setText("错误");
    }
    ElaText* subTitle = new ElaText(msg, this);
    subTitle->setTextStyle(ElaTextType::Body);

    vLayout->addWidget(titleText);
    vLayout->addSpacing(2);
    vLayout->addWidget(subTitle);
    vLayout->addStretch();

    auto buttonLayout = new QHBoxLayout;
    auto acceptButton = new ElaPushButton("确定", this);
    connect(acceptButton, &QPushButton::clicked, this, [this]() { this->accept(); });
    auto rejectButton = new ElaPushButton("取消", this);
    connect(rejectButton, &QPushButton::clicked, this, [this]() { this->reject(); });
    buttonLayout->addWidget(rejectButton);
    buttonLayout->addWidget(acceptButton);
    vLayout->addLayout(buttonLayout);
}

XElaDialog::~XElaDialog() {}

void XElaDialog::showCentered()
{
    // 将弹窗移到当前屏幕中心
    moveToCurrentScreen();
    // 调用 exec 显示模态弹窗
    exec();
}

void XElaDialog::showEvent(QShowEvent* event)
{
    // 父类处理
    ElaDialog::showEvent(event);

    // 每次显示时都定位到当前屏幕中心
    moveToCurrentScreen();
}

void XElaDialog::moveToCurrentScreen()
{
    // 获取当前鼠标位置
    QPoint cursorPos = QCursor::pos();

    // 找到鼠标所在的屏幕
    QScreen* currentScreen = nullptr;
    for (QScreen* screen : QGuiApplication::screens())
    {
        if (screen->geometry().contains(cursorPos))
        {
            currentScreen = screen;
            break;
        }
    }

    // 如果找不到（不应该发生），使用主屏幕
    if (!currentScreen)
    {
        currentScreen = QGuiApplication::primaryScreen();
    }

    // 获取屏幕的有效昺示区域（排除任务栏一些区域）
    QRect screenGeometry = currentScreen->geometry();

    // 计算弹窗中心位置
    int x = screenGeometry.left() + (screenGeometry.width() - this->width()) / 2;
    int y = screenGeometry.top() + (screenGeometry.height() - this->height()) / 2;

    // 移动弹窗
    this->move(x, y);

    qDebug() << "弹窗定位 - 屏幕:" << currentScreen->name() << "位置(" << x << "," << y << ") "
             << "屏幕哲学:" << screenGeometry;
}
