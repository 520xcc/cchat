#ifndef CHATUSERLIST_H
#define CHATUSERLIST_H
#include <QListWidget>
#include <QWheelEvent>
#include <QEvent>
#include <QScrollBar>
#include <QDebug>

class ChatUserList:public QListWidget
{
    Q_OBJECT
public:
    ChatUserList(QWidget *parent = nullptr);
protected:
    //事件过滤器：鼠标滑动列表会显示侧边滚动条，滚动快慢（步长）、样式
    bool eventFilter(QObject *watched,QEvent *event) override;
private:
    bool _load_pending;

signals:
    void sig_loading_chat_user();
};

#endif // CHATUSERLIST_H
