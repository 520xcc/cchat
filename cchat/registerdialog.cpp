#include "registerdialog.h"
#include "ui_registerdialog.h"
#include "global.h"
#include "httpmgr.h"
#include "clickedlabel.h"
#include <QMouseEvent>

RegisterDialog::RegisterDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RegisterDialog),
    _countdown(5)
{
    ui->setupUi(this);
    //这两个输入框设置成密码格式
    ui->pass_edit->setEchoMode(QLineEdit::Password);
    ui->confirm_edit->setEchoMode(QLineEdit::Password);

    //登录窗口将err_tip放进去便于后面找到要刷新的对象，global中定义了刷新操作
    //在实现这个逻辑之前需要让QT知道有两种属性可以切换，这里初始化为normal状态
    ui->err_tip->setProperty("state", "normal");
    repolish(ui->err_tip);

    //在注册模块连接http发来的通知，告诉注册模块收到了回包。
    connect(HttpMgr::GetInstance().get(), &HttpMgr::sig_reg_mod_finish, this, &RegisterDialog::slot_reg_mod_finish);
    //http发来注册完成信号给注册模块，注册模块调用槽函数实现

    initHttpHandlers();

    //在前端检查所有的错误，设定输入框输入后清空字符串
    ui->err_tip->clear();

    connect(ui->user_edit,&QLineEdit::editingFinished,this,[this](){
        checkUserValid();
    });

    connect(ui->email_edit, &QLineEdit::editingFinished, this, [this](){
        checkEmailValid();
    });

    connect(ui->pass_edit, &QLineEdit::editingFinished, this, [this](){
        checkPassValid();
    });

    connect(ui->confirm_edit, &QLineEdit::editingFinished, this, [this](){
        checkPassValid();
    });

    connect(ui->verify_edit, &QLineEdit::editingFinished, this, [this](){
            checkVarifyValid();
    });

    ui->pass_visible->setCursor(Qt::PointingHandCursor);
    ui->confirm_visible->setCursor(Qt::PointingHandCursor);


    //设置浮动显示手形状
    ui->pass_visible->setCursor(Qt::PointingHandCursor);
    ui->confirm_visible->setCursor(Qt::PointingHandCursor);

    ui->pass_visible->SetState("unvisible","unvisible_hover","","visible",
                                "visible_hover","");

    ui->confirm_visible->SetState("unvisible","unvisible_hover","","visible",
                                    "visible_hover","");
    //连接点击事件
    //触发槽函数可见或者不可见
    connect(ui->pass_visible, &ClickedLabel::clicked, this, [this]() {
        auto state = ui->pass_visible->GetCurState();
        if(state == ClickLbState::Normal){
            ui->pass_edit->setEchoMode(QLineEdit::Password);
        }else{
                ui->pass_edit->setEchoMode(QLineEdit::Normal);
        }
        qDebug() << "Label was clicked!";
    });

    connect(ui->confirm_visible, &ClickedLabel::clicked, this, [this]() {
        auto state = ui->confirm_visible->GetCurState();
        if(state == ClickLbState::Normal){
            ui->confirm_edit->setEchoMode(QLineEdit::Password);
        }else{
                ui->confirm_edit->setEchoMode(QLineEdit::Normal);
        }
        qDebug() << "Label was clicked!";
    });

    //初始化创建一个定时器
    _countdown_timer = new QTimer(this);

    //连接槽函数，到时间就会切换界面
    connect(_countdown_timer, &QTimer::timeout, [this](){
        if(_countdown==0){
            _countdown_timer->stop();
            emit sigSwitchLogin();
            return;
        }
        _countdown--;
        auto str = QString("注册成功，%1 s后返回登录").arg(_countdown);
        ui->tip_lb->setText(str);
    });
}

RegisterDialog::~RegisterDialog()
{
    qDebug()<<"desturt RegisterDialog";
    delete ui;
}

//点击获取验证码后发信号给注册模块的槽函数实现
void RegisterDialog::on_get_code_clicked()
{
    //获取到ui的文本
    auto email = ui->email_edit->text();
    //获取到后对比文本和正则表达式
    QRegularExpression regex(R"((\w+)(\./_)?(\w*)@(\w+)(\.(\w+))+)");//\w+表示匹配一个或多个字符
    bool match = regex.match(email).hasMatch();
    if(match){
        //发送http验证码
        QJsonObject json_obj;
        json_obj["email"] = email;//匹配就放入这个字段中

//        HttpMgr::GetInstance()->PostHttpReq(QUrl("http://localhost:8080/get_varifycode"),
//                            json_obj, ReqId::ID_GET_VARIFY_CODE,Modules::REGISTERMOD);

        qDebug()<< "gate_url_prefix" << gate_url_prefix;
        HttpMgr::GetInstance()->PostHttpReq(QUrl(gate_url_prefix+"/get_varifycode"),
                            json_obj, ReqId::ID_GET_VARIFY_CODE,Modules::REGISTERMOD);
        //参数说明：url、email、功能、模块
    }else{
        //写一个函数做提示
        showTip(tr("邮箱地址不正确"),false);
    }
}
//注册获取验证码的处理函数
void RegisterDialog::initHttpHandlers()
{
    //注册部分获取验证码回包逻辑
    _handlers.insert(ReqId::ID_GET_VARIFY_CODE, [this](const QJsonObject& jsonObj){//id -返回- lambda表达式
        int error = jsonObj["error"].toInt();//如果出错key="error"-->value对于int类型，返回给int error
        //如果错得出的误码表示不成功
        if(error != ErrorCodes::SUCCESS){
            showTip(tr("参数错误"),false);
            return;
        }
        //成功的话继续解析
        auto email = jsonObj["email"].toString();
        showTip(tr("验证码已发送到邮箱，注意查收"),true);
        qDebug()<< "email is " << email ;//原本需要写到日志，先打印到控制台
    });

    //注册部分注册用户回包逻辑
    _handlers.insert(ReqId::ID_REG_USER, [this](QJsonObject jsonObj){
        int error = jsonObj["error"].toInt();
        if(error != ErrorCodes::SUCCESS){
            showTip(tr("参数错误"),false);
            return;
        }
        auto email = jsonObj["email"].toString();
        showTip(tr("用户注册成功"), true);
        qDebug()<< "user uuid is " << jsonObj["uid"].toString();
        qDebug()<< "email is " << email ;
        ChangeTipPage();
    });

}

//checkUser查看用户有效
void RegisterDialog::AddTipErr(TipErr te, QString tips)
{
    _tip_errs[te] = tips;
    showTip(tips, false);
}

void RegisterDialog::DelTipErr(TipErr te)
{
    _tip_errs.remove(te);
    if(_tip_errs.empty()){
      ui->err_tip->clear();
      return;
    }

    showTip(_tip_errs.first(), false);
}

void RegisterDialog::ChangeTipPage()
{
    _countdown_timer->stop();
    ui->stackedWidget->setCurrentWidget(ui->page_2);

    // 启动定时器，设置间隔为1000毫秒（1秒）
    _countdown_timer->start(1000);

}

bool RegisterDialog::checkUserValid()
{
    if(ui->user_edit->text() == ""){
            AddTipErr(TipErr::TIP_USER_ERR, tr("用户名不能为空"));
            return false;
        }

        DelTipErr(TipErr::TIP_USER_ERR);
        return true;
}

bool RegisterDialog::checkEmailValid()
{
    //验证邮箱的地址正则表达式
        auto email = ui->email_edit->text();
        // 邮箱地址的正则表达式
        QRegularExpression regex(R"((\w+)(\.|_)?(\w*)@(\w+)(\.(\w+))+)");
        bool match = regex.match(email).hasMatch(); // 执行正则表达式匹配
        if(!match){
            //提示邮箱不正确
            AddTipErr(TipErr::TIP_EMAIL_ERR, tr("邮箱地址不正确"));
            return false;
        }

        DelTipErr(TipErr::TIP_EMAIL_ERR);
        return true;
}

bool RegisterDialog::checkPassValid()
{
    auto pass = ui->pass_edit->text();
    auto confirm = ui->confirm_edit->text();
    if(pass.length() < 6 || pass.length() > 15){
        //提示长度不准确
        AddTipErr(TipErr::TIP_PWD_ERR, tr("密码长度应为6~15"));
        return false;
    }

    // 创建一个正则表达式对象，按照上述密码要求
    // 这个正则表达式解释：
    // ^[a-zA-Z0-9!@#$%^&*]{6,15}$ 密码长度至少6，可以是字母、数字和特定的特殊字符
    QRegularExpression regExp("^[a-zA-Z0-9!@#$%^&*]{6,15}$");
    bool match = regExp.match(pass).hasMatch();
    if(!match){
        //提示字符非法
        AddTipErr(TipErr::TIP_PWD_ERR, tr("不能包含非法字符"));
        return false;
    }

    DelTipErr(TipErr::TIP_PWD_ERR);

    if(pass != confirm){
        //提示密码不匹配
        AddTipErr(TipErr::TIP_PWD_CONFIRM, tr("密码与确认密码不一致"));
        return false;
    }else{
        DelTipErr(TipErr::TIP_PWD_CONFIRM);
    }
    return true;
}

bool RegisterDialog::checkVarifyValid()
{
    auto pass = ui->verify_edit->text();
    if(pass.isEmpty()){
        AddTipErr(TipErr::TIP_VARIFY_ERR, tr("验证码不能为空"));
        return false;
    }

    DelTipErr(TipErr::TIP_VARIFY_ERR);
    return true;
}



//展示错误提示的函数的实现
void RegisterDialog::showTip(QString str, bool b_ok)
{
    if(b_ok){
        ui->err_tip->setProperty("state", "normal");
    }else{
        ui->err_tip->setProperty("state", "err");
    }
    //错误提示获取到文本，判断是否匹配，发现匹配不正确后先修改状态为红色
    ui->err_tip->setText(str);

    //然后为了显示状态还需要刷新一下
    repolish(ui->err_tip);
}


//http模块的工作
//接收注册结束响应的槽函数的实现
void RegisterDialog::slot_reg_mod_finish(ReqId id, QString res, ErrorCodes err)
{
    if(err != ErrorCodes::SUCCESS){
        showTip(tr("网络请求错误"),false);
        return;
    }
    // 解析 JSON 字符串,res需转化为QByteArray（json文档）
    QJsonDocument jsonDoc = QJsonDocument::fromJson(res.toUtf8());
    //json解析失败
    if(jsonDoc.isNull()){
        showTip(tr("json解析失败"),false);
        return;
    }
    //json解析错误
    if(!jsonDoc.isObject()){
        showTip(tr("json解析错误"),false);
        return;//后续补充日志
    }
    //调用对应的逻辑,根据id回调。//转为json对象 jsonDoc.object();
    _handlers[id](jsonDoc.object());

    //调用对应的逻辑
    return;
}


void RegisterDialog::on_sure_btn_clicked()
{
    if(ui->user_edit->text() == ""){
            showTip(tr("用户名不能为空"), false);
            return;
        }

        if(ui->email_edit->text() == ""){
            showTip(tr("邮箱不能为空"), false);
            return;
        }

        if(ui->pass_edit->text() == ""){
            showTip(tr("密码不能为空"), false);
            return;
        }

        if(ui->confirm_edit->text() == ""){
            showTip(tr("确认密码不能为空"), false);
            return;
        }

        if(ui->confirm_edit->text() != ui->pass_edit->text()){
            showTip(tr("密码和确认密码不匹配"), false);
            return;
        }

        if(ui->verify_edit->text() == ""){
            showTip(tr("验证码不能为空"), false);
            return;
        }

        //day11 发送http请求注册用户
        QJsonObject json_obj;
        json_obj["user"] = ui->user_edit->text();
        json_obj["email"] = ui->email_edit->text();
        json_obj["passwd"] = xorString(ui->pass_edit->text());
        json_obj["confirm"] = xorString(ui->confirm_edit->text());
        json_obj["varifycode"] = ui->verify_edit->text();
        HttpMgr::GetInstance()->PostHttpReq(QUrl(gate_url_prefix+"/user_register"),
                     json_obj, ReqId::ID_REG_USER,Modules::REGISTERMOD);
}

void RegisterDialog::on_return_btn_clicked()
{
    _countdown_timer->stop();
    emit sigSwitchLogin();
}

void RegisterDialog::on_cancel_btn_clicked()
{
    _countdown_timer->stop();
    emit sigSwitchLogin();
}
