#ifndef APPLYFRIENDITEM_H
#define APPLYFRIENDITEM_H
#include <QWidget>
#include "listitembase.h"
#include "userdata.h"
#include <memory>

namespace Ui {
class ApplyFriendItem;
}

class ApplyFriendItem : public ListItemBase
{
    Q_OBJECT

public:
    explicit ApplyFriendItem(QWidget *parent = nullptr);
    ~ApplyFriendItem();
    void SetInfo(std::shared_ptr<ApplyInfo> apply_info);
    void ShowAddBtn(bool bshow);
    QSize sizeHint() const override{
      return QSize(250,80);
    }
    int GetUid();
private:
    Ui::ApplyFriendItem *ui;
    std::shared_ptr<ApplyInfo> _apply_info;
    bool _added;
signals:
    //点击添加按钮会发送这个信号,告诉tcpmgr发送认证通过请求给后台服务器告诉它处理后续
    void sig_auth_friend(std::shared_ptr<ApplyInfo> apply_info);
};

#endif // APPLYFRIENDITEM_H
