#pragma once

#include "ElaDialog.h"

enum class XElaDialogType
{
    INFO,
    ERR
};

/**
 * @brief 自定义弹窗类，支持多屏幕中心显示
 */
class XElaDialog : public ElaDialog
{
    Q_OBJECT

public:
    XElaDialog(QString msg, XElaDialogType type, QWidget* parent = nullptr);
    ~XElaDialog();

    /**
     * @brief 在当前鼠标所在的屏幕中心显示弹窗
     */
    void showCentered();

protected:
    /**
     * @brief 重写 showEvent 以确保每次显示时都在屏幕中心
     */
    void showEvent(QShowEvent* event) override;

private:
    /**
     * @brief 将弹窗移到鼠标所在的屏幕中心
     */
    void moveToCurrentScreen();
};
