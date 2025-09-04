#include "findfaildlg.h"
#include "ui_findfaildlg.h"
#include <QDebug>

FindFailDlg::FindFailDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FindFailDlg)
{
    ui->setupUi(this);
    //小标题
    setWindowTitle("添加");
    //隐藏工具栏边框
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
    this->setObjectName("FindFailDlg");
    //设置btn的三个状态
    ui->fail_sure_btn->SetState("normal","hover","press");
    setModal(true);
}

FindFailDlg::~FindFailDlg()
{
    qDebug() << "Find FailDlg destruct";
    delete ui;
}

void FindFailDlg::on_fail_sure_btn_clicked()
{
    this->hide();
}
