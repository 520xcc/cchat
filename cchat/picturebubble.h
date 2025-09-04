#ifndef PICTUREBUBBLE_H
#define PICTUREBUBBLE_H
#include "bubbleframe.h"
#include <QHBoxLayout>
#include <QPixmap>


class PictureBubble: public BubbleFrame
{
    Q_OBJECT
public:
    //只需要构造即可
    PictureBubble(const QPixmap &picture,ChatRole role,QWidget *parrent = nullptr);
};
#endif // PICTUREBUBBLE_H
