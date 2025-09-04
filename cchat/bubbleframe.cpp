#include "bubbleframe.h"
#include <QPainter>
#include <QDebug>

const int WIDTH_SANJIAO = 8;//发送气泡的那个小三角的宽度

BubbleFrame::BubbleFrame(ChatRole role, QWidget *parent): QFrame(parent),m_role(role),m_margin(3)
{
    m_pHLayout = new QHBoxLayout();
    if(m_role == ChatRole::Self)
        m_pHLayout->setContentsMargins(m_margin, m_margin, WIDTH_SANJIAO + m_margin, m_margin);
        //间距。注意self和other的三角形宽度分别加在右边和左边
    else
        m_pHLayout->setContentsMargins(WIDTH_SANJIAO + m_margin, m_margin, m_margin, m_margin);

    this->setLayout(m_pHLayout);
}

void BubbleFrame::setMargin(int margin)
{
    Q_UNUSED(margin)
    //    _margin = margin;
}

//Widget加入布局，用来放发送的内容
void BubbleFrame::setWidget(QWidget *w)
{
    if (m_pHLayout->count() > 0){
        return;
    }
    else{
        m_pHLayout ->addWidget(w);
    }
}
//绘制气泡框
void BubbleFrame::paintEvent(QPaintEvent *e)
{
    QPainter painter(this);
    painter.setPen(Qt::NoPen);//气泡无边框

    if(m_role == ChatRole::Other)
    {
        //画气泡
        QColor bk_color(Qt::white);
        painter.setBrush(QBrush(bk_color));
        QRect bk_rect = QRect(WIDTH_SANJIAO, 0, this->width()-WIDTH_SANJIAO, this->height());
        painter.drawRoundedRect(bk_rect,5,5);
        //画小三角
        QPointF points[3] = {
            QPointF(bk_rect.x(), 12),
            QPointF(bk_rect.x(), 10+WIDTH_SANJIAO +2),
            QPointF(bk_rect.x()-WIDTH_SANJIAO, 10+WIDTH_SANJIAO-WIDTH_SANJIAO/2),
        };
        painter.drawPolygon(points, 3);
    }
    else
    {
        //自己发
        QColor bk_color(158,234,106);//绿色
        painter.setBrush(QBrush(bk_color));

        //画气泡（减去三角的宽度）
        QRect bk_rect = QRect(0, 0, this->width()-WIDTH_SANJIAO, this->height());//左上角是0,0的坐标，气泡绘制的起始点
        painter.drawRoundedRect(bk_rect,5,5);

        //画三角（里面是三角形的三个点）
        QPointF points[3] = {
            QPointF(bk_rect.x()+bk_rect.width(), 12),
            QPointF(bk_rect.x()+bk_rect.width(), 12+WIDTH_SANJIAO +2),
            QPointF(bk_rect.x()+bk_rect.width()+WIDTH_SANJIAO, 10+WIDTH_SANJIAO-WIDTH_SANJIAO/2),
        };
        painter.drawPolygon(points, 3);
    }
    return QFrame::paintEvent(e);
}
