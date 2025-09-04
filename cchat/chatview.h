#ifndef CHATVIEW_H
#define CHATVIEW_H
#include <QScrollArea>
#include <QVBoxLayout>
#include <QTimer>

//聊天具体信息界面。不用ui，而是手写版本
class ChatView:public QWidget
{
    Q_OBJECT
public:
    ChatView(QWidget* parent = Q_NULLPTR);
    //尾插
    void appendChatItem(QWidget *item);
    //头插
    void prependChatItem(QWidget *item);
    //中间插
    void insertChatItem(QWidget *before,QWidget *item);
    void removeAllItem();
protected:
    //事件过滤器
    bool eventFilter(QObject *o,QEvent *e) override;
    //喷绘
    void paintEvent(QPaintEvent *event) override;

private slots:
    //滑动槽函数
    void onVScrollBarMoved(int min,int max);

private:
    //初始化样式表
    void initStyleSheet();

private:
    //QWidget *m_pCenterWidget;
    //垂直布局
    QVBoxLayout *m_pVl;
    //滚动区域
    QScrollArea *m_pScrollArea;
    bool isAppended;   //标志是否正在加载，为true则正在加载
};

#endif // CHATVIEW_H
