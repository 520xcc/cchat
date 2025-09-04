#ifndef TIMERBTN_H
#define TIMERBTN_H
#include <QPushButton>
#include <QTimer>

class TimerBtn : public QPushButton
{
public:
    TimerBtn(QWidget *parent = nullptr);
    ~TimerBtn();

    // 重写mouseReleaseEvent，鼠标抬起事件（override覆盖）
    virtual void mouseReleaseEvent(QMouseEvent *e) override;
private:
    QTimer  *_timer;//定时器
    int _counter;//计数
};

#endif // TIMERBTN_H
