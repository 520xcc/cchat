#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "resetdialog.h"

//在mainwindow.h中引入logindialog.h头文件->后面展示要用

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    //登录界面（初始化）
    //主函数中new创建这个logindlg指针对象
    _login_dlg = new LoginDialog(this);
    _login_dlg->setWindowFlags(Qt::CustomizeWindowHint|Qt::FramelessWindowHint);

    setCentralWidget(_login_dlg);

    //两个对象初始化完成后，要设置一个信号和槽，->点击注册按钮->跳转
    //创建和消息链接
    connect(_login_dlg, &LoginDialog::switchRegister, this, &MainWindow::SlotSwitchReg);
    //信号是由LoginDialog 的对象_login_dlg发起切换switchRegister信号到登陆界面，切换操作由mainwindow来做（需要写一个槽函数）

    //连接登录界面忘记密码信号
    connect(_login_dlg, &LoginDialog::switchReset, this, &MainWindow::SlotSwitchReset);

    //连接：tcp连接完成触发的信号 sig_swich_chatdlg（通知跳转到聊天界面）-->跳转到聊天页面的槽函数
    connect(TcpMgr::GetInstance().get(),&TcpMgr::sig_swich_chatdlg, this, &MainWindow::SlotSwitchChat);

//    //链接服务器踢人消息
//    connect(TcpMgr::GetInstance().get(),&TcpMgr::sig_notify_offline, this, &MainWindow::SlotOffline);
//    //连接服务器断开心跳超时或异常连接信息
//    connect(TcpMgr::GetInstance().get(),&TcpMgr::sig_connection_closed, this, &MainWindow::SlotExcepConOffline);
    //测试用
    //emit TcpMgr::GetInstance()->sig_swich_chatdlg();

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::SlotSwitchReg(){
    //动态初始化
    //注册界面（初始化）
    _reg_dlg = new RegisterDialog(this);
    _reg_dlg->hide();//隐藏一下register初始画面
    //用户自定义窗口
    _reg_dlg->setWindowFlags(Qt::CustomizeWindowHint|Qt::FramelessWindowHint);
    //设置自定义样式，不是最大化最小化样式，按位或表示两个属性都需要满足，还需要把边框设没->这时候不需要show()

    //连接注册界面返回登录信号
    connect(_reg_dlg, &RegisterDialog::sigSwitchLogin, this, &MainWindow::SlotSwitchLogin);

    //界面设置成注册界面，登陆界面隐藏
    setCentralWidget(_reg_dlg);
    _login_dlg->hide();
    //展示注册界面
    _reg_dlg->show();
}

void MainWindow::SlotSwitchLogin()
{
    //创建一个CentralWidget, 并将其设置为MainWindow的中心部件
    _login_dlg = new LoginDialog(this);
    _login_dlg->setWindowFlags(Qt::CustomizeWindowHint|Qt::FramelessWindowHint);
    setCentralWidget(_login_dlg);

    _reg_dlg->hide();
    _login_dlg->show();
    //连接登录界面注册信号
    connect(_login_dlg, &LoginDialog::switchRegister, this, &MainWindow::SlotSwitchReg);
    //连接登录界面忘记密码信号
    connect(_login_dlg, &LoginDialog::switchReset, this, &MainWindow::SlotSwitchReset);

}

void MainWindow::SlotSwitchReset()
{
    //创建一个CentralWidget, 并将其设置为MainWindow的中心部件
    _reset_dlg = new ResetDialog(this);
    _reset_dlg->setWindowFlags(Qt::CustomizeWindowHint|Qt::FramelessWindowHint);
    setCentralWidget(_reset_dlg);

    _login_dlg->hide();
    _reset_dlg->show();
    //注册返回登录信号和槽函数
    connect(_reset_dlg, &ResetDialog::switchLogin, this, &MainWindow::SlotSwitchLogin2);
}

void MainWindow::SlotSwitchLogin2()
{
    //创建一个CentralWidget, 并将其设置为MainWindow的中心部件
        _login_dlg = new LoginDialog(this);
        _login_dlg->setWindowFlags(Qt::CustomizeWindowHint|Qt::FramelessWindowHint);
        setCentralWidget(_login_dlg);

       _reset_dlg->hide();
        _login_dlg->show();
        //连接登录界面忘记密码信号
        connect(_login_dlg, &LoginDialog::switchReset, this, &MainWindow::SlotSwitchReset);
        //连接登录界面注册信号
        connect(_login_dlg, &LoginDialog::switchRegister, this, &MainWindow::SlotSwitchReg);
}

void MainWindow::SlotSwitchChat()
{
    _chat_dlg = new ChatDialog();
    _chat_dlg->setWindowFlags(Qt::CustomizeWindowHint|Qt::FramelessWindowHint);
    setCentralWidget(_chat_dlg);
    _chat_dlg->show();
    _login_dlg->hide();
    this->setMinimumSize(QSize(1050,900));
    this->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
}

////展示聊天界面
//void MainWindow::SlotSwitchChat()
//{
//    _chat_dlg = new ChatDialog();
//    _chat_dlg->setWindowFlags(Qt::CustomizeWindowHint|Qt::FramelessWindowHint);
//    setCentralWidget(_chat_dlg);
//    _chat_dlg->show();
//    _login_dlg->hide();
//    this->setMinimumSize(QSize(1050,900));
//    this->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
//    //_ui_status = CHAT_UI;
//}

