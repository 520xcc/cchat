#ifndef CLICKEDBTN_H
#define CLICKEDBTN_H

#include <QPushButton>
#include "global.h"

class ClickedBtn:public QPushButton
{
    Q_OBJECT
public:
    ClickedBtn(QWidget* parent = nullptr);
    ~ClickedBtn();
    //设置添加好友按钮三个状态
    void SetState(QString normal,QString hover,QString press);
protected:
    //protected 意味着 类自己 和 其子类 都能访问这些函数
    //这样，如果以后你基于 ClickedBtn 再派生一个更特殊的按钮类，就可以在子类中重写这些事件处理函数
    virtual void mousePressEvent(QMouseEvent *event) override;//鼠标按下
    virtual void mouseReleaseEvent(QMouseEvent *event) override;//鼠标释放
    virtual void enterEvent(QEvent *event) override;//鼠标进入
    virtual void leaveEvent(QEvent *event) override;//鼠标离开
private:
    QString _normal;
    QString _hover;
    QString _press;
};
#endif // CLICKEDBTN_H
