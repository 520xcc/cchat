#include "timerbtn.h"
#include <QMouseEvent>
#include <QDebug>

TimerBtn::TimerBtn(QWidget *parent):QPushButton(parent),_counter(30)//倒计时
{
    _timer = new QTimer(this);//定时器

    connect(_timer, &QTimer::timeout, [this](){//定时器连接信号和槽，定时器达到超时时间
        _counter--;
        if(_counter <= 0){
            _timer->stop();
            _counter = 30;
            this->setText("获取");
            this->setEnabled(true);
            return;
        }
        this->setText(QString::number(_counter));
    });
}

TimerBtn::~TimerBtn()
{
    _timer->stop();
}

void TimerBtn::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
        // 在这里处理鼠标左键释放事件
        qDebug() << "MyButton was released!";
        this->setEnabled(false);
         this->setText(QString::number(_counter));
        _timer->start(1000);
        emit clicked();//继承的信号，发给外部我们能够看见
    }
    // 调用基类的mouseReleaseEvent以确保正常的事件处理（如点击效果）
    QPushButton::mouseReleaseEvent(e);
}
