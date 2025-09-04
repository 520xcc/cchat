#ifndef HTTPMGR_H
#define HTTPMGR_H
#include "singleton.h"
#include <QString>
#include <QUrl>
#include <QObject>//信号传递消息
#include <QNetworkAccessManager>  //网络管理：这里需要使用QT的网络
#include <QJsonObject>    //发送数据->序列化成字节流
#include <QJsonDocument>  //反序列化->解析json

//HttpMgr是Singleton的子类，子类可以构造
//自己还没有实现，先使用模板类实例了自己（CRTP：只有在编译运行时才会动态实例）
class HttpMgr:public QObject, public Singleton<HttpMgr>, public std::enable_shared_from_this<HttpMgr>
{
    Q_OBJECT  //宏
public:
    ~HttpMgr();
    //Singleton返回了一个实例_instance，是T类型，是Singleton的一个成员，在执行析构的时候，会回收自己的成员变量。
    //回收_instance的时候会调用它的析构，也就是智能指针的析构
    //在智能指针内部会调用T类型的析构，这里T就是httpmgr。因此这里要被置为公有的析构函数
    //定义一个发送的函数，post请求
    void PostHttpReq(QUrl url, QJsonObject json, ReqId req_id, Modules mod);//思考发送的参数：地址、一串json数据
    //需要知道哪个模块,请求什么内容（参数透传）请求id

private:
    //_instance = std::shared_ptr<T>(new T);在Singleton中调用httpmgr的构造是不允许的-->声明友元
    //这样基类就可以访问子类的构造函数
    friend class Singleton<HttpMgr>;
    HttpMgr();

    //要使用http发送的功能，需要定义一个httpmgr，原生的http网络管理者。
    QNetworkAccessManager _manager;
private slots:
    //写一个私有的槽函数（接收到响应结束信号后，调用槽函数来接收sig_http_finish信号。）
    void slot_http_finish(ReqId id, QString res, ErrorCodes err, Modules mod);


signals:
    //http发送完成后，会发这个信号给其他的模块
    void sig_http_finish(ReqId id, QString res, ErrorCodes err, Modules mod);//添加返回的信息，包括请求id、返回的数据、错误码

    //注册模块收到结束响应信号，注册模块连接它
    void sig_reg_mod_finish(ReqId id, QString res, ErrorCodes err);

    //重置密码发送了这个请求后收到的响应信号
    void sig_reset_mod_finish(ReqId id, QString res, ErrorCodes err);

    //登录模块收到结束响应信号，登录模块连接它
    void sig_login_mod_finish(ReqId id, QString res, ErrorCodes err);
};

#endif // HTTPMGR_H
