#ifndef TCPMGR_H
#define TCPMGR_H
#include <QTcpSocket>
#include "singleton.h"
#include "global.h"
#include <functional>
#include <QObject>
#include "userdata.h"
#include <QJsonArray>

//QObject发送信号，接收信号
class TcpMgr:public QObject, public Singleton<TcpMgr>,
        public std::enable_shared_from_this<TcpMgr>
{
    Q_OBJECT
public:
    ~TcpMgr();
    void CloseConnection();
private:
    //声明友元类，可以允许访问tcpmgr的构造
    friend class Singleton<TcpMgr>;
    TcpMgr();
    //注册回调处理（请求id与回调绑在一起）
    void initHandlers();
    //利用三个参数处理数据-->找到reqid对应的回调，调用回调
    void handleMsg(ReqId id, int len, QByteArray data);

    QTcpSocket _socket;
    QString _host;
    uint16_t _port;
    QByteArray _buffer;
    bool _b_recv_pending;
    quint16 _message_id;
    quint16 _message_len;
    QMap<ReqId, std::function<void(ReqId id, int len, QByteArray data)>> _handlers;
public slots:
    //连接成功触发的槽函数
    void slot_tcp_connect(ServerInfo);
    //发送数据也会调用槽函数
    void slot_send_data(ReqId reqId, QByteArray data);
signals:
    //连接成功/发送数据/切换界面/登陆失败信号->告诉其他界面
    void sig_con_success(bool bsuccess);
    void sig_send_data(ReqId reqId, QByteArray data);
    void sig_swich_chatdlg();
    void sig_load_apply_list(QJsonArray json_array);
    void sig_login_failed(int);
    void sig_user_search(std::shared_ptr<SearchInfo>);

    //对方发来的好友申请
    void sig_friend_apply(std::shared_ptr<AddFriendApply>);
    //我发的申请对方同意了
    void sig_add_auth_friend(std::shared_ptr<AuthInfo>);
    //对方发来的好友申请我同意了
    void sig_auth_rsp(std::shared_ptr<AuthRsp>);

    //发信息
    void sig_text_chat_msg(std::shared_ptr<TextChatMsg> msg);
//    void sig_notify_offline();
//    void sig_connection_closed();
};

#endif // TCPMGR_H
