#ifndef LISTITEMBASE_H
#define LISTITEMBASE_H
#include <QWidget>
#include "global.h"

//条目的基类，供子类条目继承。
class ListItemBase:public QWidget
{
    Q_OBJECT
public:
    explicit ListItemBase(QWidget *parent = nullptr);//显示声明显示构造，也就是不可以走默认构造
    void SetItemType(ListItemType itemType);

    ListItemType GetItemType();
protected:
    virtual void paintEvent(QPaintEvent *event) override;

private:
    ListItemType _itemType;

public slots:

signals:

};

#endif // LISTITEMBASE_H
