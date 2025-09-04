#include "httpmgr.h"
#include "global.h"

HttpMgr::~HttpMgr(){}

HttpMgr::HttpMgr()
{
    //连接http请求和完成信号，信号槽机制保证队列消费，连接信号和槽
    connect(this, &HttpMgr::sig_http_finish, this, &HttpMgr::slot_http_finish);
}

//实现PostHttpReq，也就是http请求、接收对方的回包、将回包信息透传给其他界面功能：
void HttpMgr::PostHttpReq(QUrl url, QJsonObject json, ReqId req_id, Modules mod)
{
    //json-序列化->字符串
    //创建一个HTTP POST请求，并设置请求头和请求体
    QByteArray data = QJsonDocument(json).toJson();
    //通过url构造请求
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");//请求类型
    request.setHeader(QNetworkRequest::ContentLengthHeader, QByteArray::number(data.length()));//data长度传给QByteArray::number，通过计算返回计算的结果
    //发送请求，并处理响应, 获取自己的智能指针，构造伪闭包并增加智能指针引用计数
    auto self = shared_from_this();//http的this指针生成的智能对象-->扔给lambda表达式，防止lambda没有调用完http的时候httpmgr被回收了。
    //外部使用智能指针管理，内部也要根据this生成智能指针self，内部生成的智能指针与外部共用引用计数，不会出现两个智能指针引用计数不同步


    QNetworkReply * reply = _manager.post(request, data);//发送完请求（_manager.post,将request和具体数据发过去）会收到响应，返回reply的指针。后期要回收

    //设置信号和槽等待发送完成：当收到回复包后，reply发出信号finished，收到信号后给PostHttpReq，这里写一个lambda表达式（肯定会用到httpmgr中的数据），捕获对象不能直接写this。
    //不能保证在触发回调之前出现httpmgr被回收的情况，否则可能会导致第一次回调（会用到httpmgr中数据）之前，httpmgr被删掉，再次触发回调区里面的数据会出现数据混乱、崩溃的情况
    QObject::connect(reply, &QNetworkReply::finished, [self, reply, req_id, mod](){//这里使用self会对引用计数+1,那么外层释放掉的时候这里还有智能指针管理着
        //处理错误的情况
        if(reply->error() != QNetworkReply::NoError){
            qDebug() << reply->errorString(); //打印错误信息
            //发送信号给其他窗口通知完成
            emit self->sig_http_finish(req_id, "", ErrorCodes::ERR_NETWORK, mod);//出错也要告诉其他界面
            reply->deleteLater();//回收reply
            return;
        }
        //无错误则读回请求
        QString res = reply->readAll();
        //发送信号通知完成
        emit self->sig_http_finish(req_id, res, ErrorCodes::SUCCESS,mod);
        reply->deleteLater();
        return;
    });
}

//实现一个slot_http_finish的槽函数用来接收sig_http_finish信号
void HttpMgr::slot_http_finish(ReqId id, QString res, ErrorCodes err, Modules mod)
{
    if(mod == Modules::REGISTERMOD){  //如果是注册模块收到了响应结束的信号。
        //发送信号通知指定模块http响应结束
        emit sig_reg_mod_finish(id, res, err);
    }

    if(mod == Modules::RESETMOD){
        //发送信号通知指定模块http响应结束
        emit sig_reset_mod_finish(id, res, err);
    }

    if(mod == Modules::LOGINMOD){
            emit sig_login_mod_finish(id, res, err);
    }
}


