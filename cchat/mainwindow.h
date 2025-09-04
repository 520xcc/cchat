#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QMainWindow>
#include "logindialog.h"
#include "registerdialog.h"
#include "resetdialog.h"
#include "chatdialog.h"
#include "tcpmgr.h"
/******************************************************************************
 *
 * @file       mainwindow.h
 * @brief      XXXX Function
 *
 * @author     心想果粒橙
 * @date       2025/07/11
 * @history
 *****************************************************************************/

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
public slots: //添加一个public槽函数
    void SlotSwitchReg();
    void SlotSwitchLogin();
    void SlotSwitchReset();
    void SlotSwitchLogin2();

    //登录成功-->跳转到聊天界面
    void SlotSwitchChat();
private:
    Ui::MainWindow *ui;
    LoginDialog *_login_dlg;  //添加loginDialog的指针变量
    RegisterDialog *_reg_dlg;  //添加registerDialog的指针变量
    ResetDialog *_reset_dlg;
    ChatDialog * _chat_dlg;
};
#endif // MAINWINDOW_H
