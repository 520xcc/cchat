#ifndef FINDFAILDLG_H
#define FINDFAILDLG_H

#include <QDialog>

namespace Ui {
class FindFailDlg;
}

class FindFailDlg : public QDialog
{
    Q_OBJECT

public:
    explicit FindFailDlg(QWidget *parent = nullptr);
    ~FindFailDlg();

private:
    Ui::FindFailDlg *ui;

private slots:
    //失败后触发槽函数
    void on_fail_sure_btn_clicked();
};

#endif // FINDFAILDLG_H
