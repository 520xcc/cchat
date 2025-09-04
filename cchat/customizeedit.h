#ifndef CUSTOMIZEEDIT_H
#define CUSTOMIZEEDIT_H
#include <QLineEdit>
#include <QDebug>

class CustomizeEdit : public QLineEdit
{
    Q_OBJECT
public:
    CustomizeEdit(QWidget *parent = nullptr);
    void SetMaxLength(int maxlen);
protected:
    //执行失去焦点时-->发信号sig_focus_out
    void focusOutEvent(QFocusEvent *event) override;
private:
    //限制最大输入长度
    void limitTextLength(QString text);

    int _max_len;
signals:
    void sig_focus_out();
};

#endif // CUSTOMIZEEDIT_H
