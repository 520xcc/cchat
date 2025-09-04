#include "chatpage.h"
#include "ui_chatpage.h"
#include "chatitembase.h"
#include "textbubble.h"
#include "picturebubble.h"
#include <QStyleOption>
#include <QPainter>
#include <QJsonDocument>
#include <QUuid>
#include "usermgr.h"
#include "tcpmgr.h"

ChatPage::ChatPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ChatPage)
{
    ui->setupUi(this);
    //设置按钮样式
    ui->receive_btn->SetState("normal","hover","press");
    ui->send_btn->SetState("normal","hover","press");

    //设置图标样式
    ui->emo_lb->SetState("normal","hover","press","normal","hover","press");
    ui->file_lb->SetState("normal","hover","press","normal","hover","press");
}

ChatPage::~ChatPage()
{
    delete ui;
}

void ChatPage::SetUserInfo(std::shared_ptr<UserInfo> user_info)
{
    _user_info = user_info;
    //设置UI界面
    ui->title_lb->setText(user_info->_name);
    ui->chat_data_list->removeAllItem();//chat_data_list就是ChatPage里的历史聊天信息列表，是ChatView类型的（这个类型我们手写的ui），移除重刷，更新最新聊天记录
    for (auto& msg : user_info->_chat_msgs){
        AppendChatMsg(msg);
    }
}

void ChatPage::AppendChatMsg(std::shared_ptr<TextChatData> msg)
{
    auto self_info = UserMgr::GetInstance()->GetUserInfo();
    ChatRole role;
    //todo... 添加聊天显示
    if (msg->_from_uid == self_info->_uid) {
        role = ChatRole::Self;//之前定义的聊天角色，自己的消息显示靠右
        ChatItemBase* pChatItem = new ChatItemBase(role);

        pChatItem->setUserName(self_info->_name);
        pChatItem->setUserIcon(QPixmap(self_info->_icon));
        QWidget* pBubble = nullptr;
        pBubble = new TextBubble(role, msg->_msg_content);
        pChatItem->setWidget(pBubble);
        ui->chat_data_list->appendChatItem(pChatItem);
    }
    else {
        role = ChatRole::Other;
        ChatItemBase* pChatItem = new ChatItemBase(role);
        auto friend_info = UserMgr::GetInstance()->GetFriendById(msg->_from_uid);
        if (friend_info == nullptr) {
            return;
        }
        pChatItem->setUserName(friend_info->_name);
        pChatItem->setUserIcon(QPixmap(friend_info->_icon));
        QWidget* pBubble = nullptr;
        pBubble = new TextBubble(role, msg->_msg_content);
        pChatItem->setWidget(pBubble);
        ui->chat_data_list->appendChatItem(pChatItem);
    }
}

void ChatPage::paintEvent(QPaintEvent *event)
{
    // 1. 创建一个样式选项对象，用于保存当前控件的绘制参数
    QStyleOption opt;
    // 2. 用当前控件(this)的状态、矩形、调色板等信息初始化 opt
    opt.initFrom(this);
    // 3. 在当前控件上创建 QPainter，准备绘图
    QPainter p(this);
    // QWIdget的Start传入 opt 提供几何/调色板，p 提供画笔，this 指定目标控件
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}
//发送的逻辑
void ChatPage::on_send_btn_clicked()
{
    //这里不可以写死-->
    if(_user_info == nullptr){
        qDebug() << "friend_info is empty";
        return;
        //用户信息如果是空就返回
    }
    //获取自己的信息
    auto user_info = UserMgr::GetInstance()->GetUserInfo();
    auto pTextEdit = ui->chatEdit;
    ChatRole role = ChatRole::Self;
    QString userName = user_info->_name;
    QString userIcon = user_info->_icon;

    //获取到用户要发的text文本信息-->组织成json发给服务器
    const QVector<MsgInfo>& msgList = pTextEdit->getMsgList();
    QJsonObject textObj;
    QJsonArray textArray;
    int txt_size = 0;
    //消息列表
    for(int i=0; i<msgList.size(); ++i)
    {
        //消息内容长度不合规就跳过
        if(msgList[i].content.length() > 1024){
            continue;
        }
        //如果内容不超过1k
        QString type = msgList[i].msgFlag;
        //构造聊天的ChatItemBase绘制聊天气泡,后面会将聊天信息贴到右侧列表中
        ChatItemBase *pChatItem = new ChatItemBase(role);
        pChatItem->setUserName(userName);
        pChatItem->setUserIcon(QPixmap(userIcon));
        //存储聊天的信息（构造一个空的pBubble）
        QWidget *pBubble = nullptr;
        //那么就取一下文本的类型
        if(type == "text")
        {
            //生成唯一id,  给每条消息都加一个id，后期方便扩展功能，比如返回消息已读、还是未读等
            QUuid uuid = QUuid::createUuid();
            //转为字符串
            QString uuidString = uuid.toString();
            //创建出文本pBubble
            pBubble = new TextBubble(role, msgList[i].content);
            //累积的文本大于1024，先把累积的发过去
            if(txt_size + msgList[i].content.length()> 1024){
                textObj["fromuid"] = user_info->_uid;
                textObj["touid"] = _user_info->_uid;
                textObj["text_array"] = textArray;
                QJsonDocument doc(textObj);
                QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
                //发送并清空之前累计的文本列表
                txt_size = 0;
                textArray = QJsonArray();
                textObj = QJsonObject();
                //发送tcp请求给chat server
                emit TcpMgr::GetInstance()->sig_send_data(ReqId::ID_TEXT_CHAT_MSG_REQ, jsonData);
            }
            //尽可能让总长度靠近1024来发
            //将bubble和uid绑定，以后可以等网络返回消息后设置是否送达
            //_bubble_map[uuidString] = pBubble;
            //取出最近要发的一条，累积到前面不满1024的长度
            txt_size += msgList[i].content.length();
            QJsonObject obj;//要发送的文本
            QByteArray utf8Message = msgList[i].content.toUtf8();//转为utf8
            obj["content"] = QString::fromUtf8(utf8Message);
            //obj["content"] = msgList[i].content;
            obj["msgid"] = uuidString;//消息id组装
            textArray.append(obj);//拼接
            auto txt_msg = std::make_shared<TextChatData>(uuidString, obj["content"].toString(),
                user_info->_uid, _user_info->_uid);
            emit sig_append_send_chat_msg(txt_msg);//把消息显示到左侧chat_item里
        }
        else if(type == "image")
        {
             pBubble = new PictureBubble(QPixmap(msgList[i].content) , role);
        }
        else if(type == "file")
        {

        }
        //发送消息，每发一条都要做一个消息拼接
        if(pBubble != nullptr)
        {
            pChatItem->setWidget(pBubble);
            ui->chat_data_list->appendChatItem(pChatItem);
        }
    }

    //比如就发一条消息，不够长度，上面循环就会退出不会发送
    qDebug() << "textArray is " << textArray ;
    //发送给服务器
    textObj["text_array"] = textArray;
    textObj["fromuid"] = user_info->_uid;
    textObj["touid"] = _user_info->_uid;
    QJsonDocument doc(textObj);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
    //发送并清空之前累计的文本列表
    txt_size = 0;
    textArray = QJsonArray();
    textObj = QJsonObject();
    //发送tcp请求给chat server
    emit TcpMgr::GetInstance()->sig_send_data(ReqId::ID_TEXT_CHAT_MSG_REQ, jsonData);
}
