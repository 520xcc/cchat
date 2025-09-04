#include "HttpConnection.h"
#include <iostream>
#include "LogicSystem.h"
#include "VerifyGrpcClient.h"
#include "RedisMgr.h"
#include "const.h"
#include "MysqlMgr.h"
#include <cstring>
#include "StatusGrpcClient.h"

//实现RegGet注册get方法
void LogicSystem::RegGet(std::string url, HttpHandler handler) {
	_get_handlers.insert(make_pair(url, handler));//键值对的形式插入
}
//实现RegPost注册post方法
void LogicSystem::RegPost(std::string url, HttpHandler handler) {
	_post_handlers.insert(make_pair(url, handler));
}
//实现构造函数，在构造函数中写
LogicSystem::LogicSystem() {
	//在构造函数中实现具体的消息注册
	RegGet("/get_test", [](std::shared_ptr<HttpConnection> connection) {
		beast::ostream(connection->_response.body()) << "receive get_test req" << std::endl;	//这里需要HttpConnection.h和LogicSystem.h相互包含
		int i = 0;
		//遍历后面的参数
		for (auto & elem : connection->_get_params) {
			i++;
			beast::ostream(connection->_response.body()) << "param" << i << " key is " << elem.first;
			beast::ostream(connection->_response.body()) << "param" << i << " value is " << elem.second << std::endl;
		}
	});
	//在构造函数里添加获取验证码的处理逻辑
	// 
	// 监听客户端对 /get_varifycode 的 POST 请求
	// 解析 JSON 请求体，提取其中的 email 字段
	// 然后调用后端服务（如 gRPC 客户端）获取验证码
	// 再构造一个 JSON 响应返回给客户端。
	// 
	//注册一个 POST 类型的 HTTP 路由，/get_varifycode 是注册的 URI 路径
	RegPost("/get_varifycode", [](std::shared_ptr<HttpConnection> connection) {
		//获取的请求转为string存入变量 body_str 中
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
		//打印收到的请求体内容（查看解析的是否正确）
		std::cout << "receive body is " << body_str << std::endl;
		//回包设置，内容类型为文本或json
		connection->_response.set(http::field::content_type, "text/json");

		//解析
		Json::Value root;    //返回给对方的时候需要构造的json结构
		Json::Reader reader; //反序列化
		Json::Value src_root;//来源的结构
		//返回是否解析成功
		bool parse_success = reader.parse(body_str, src_root);//使用reader去解析收到的请求包，解析完放到src_root中
		//判断是否成功后的操作
		if (!parse_success) {
			std::cout << "Failed to parse JSON data!" << std::endl;
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;//返回一个失败的响应，将序列化好的json串扔到body中
			return true;
		}
		//正常情况下，会发一个email来、
		//判断email不存在的逻辑
		if (!src_root.isMember("email")) {
			std::cout << "Failed to parse JSON data!" << std::endl;
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;//返回一个失败的响应，将序列化好的json串扔到body中
			return true;
		}
		//key存在，读出这个email
		auto email = src_root["email"].asString();

		//有了获取验证码的客户端，添加一个获取验证码的处理逻辑，发给varify的服务端
		GetVarifyRsp rsp = VerifyGrpcClient::GetInstance()->GetVarifyCode(email);

		std::cout << "email is " << email << std::endl;
		root["error"] = rsp.error();//0表示没有错误
		//邮箱返给对方
		root["email"] = src_root["email"];
		//不能直接发root结构，面向字节流->转为string
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;//将字符串写入body()中
		return true;
		});

	RegPost("/user_register", [](std::shared_ptr<HttpConnection> connection) {
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
		std::cout << "receive body is " << body_str << std::endl;
		connection->_response.set(http::field::content_type, "text/json");
		Json::Value root;
		Json::Reader reader;
		Json::Value src_root;
		bool parse_success = reader.parse(body_str, src_root);
		if (!parse_success) {
			std::cout << "Failed to parse JSON data!" << std::endl;
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		
		auto email = src_root["email"].asString();
		auto name = src_root["user"].asString();
		auto pwd = src_root["passwd"].asString();
		auto confirm = src_root["confirm"].asString();
		auto icon = src_root["icon"].asString();
		//密码和确认密码不一致也不行
		 
		//if (pwd != confirm) {
		//	std::cout << "password err" << std::endl;
		//	root["error"] = ErrorCodes::PasswdErr;
		//	std::string jsonstr = root.toStyledString();
		//	beast::ostream(connection->_response.body()) << jsonstr;
		//	return true;
		//}
		if (strcmp(pwd.c_str(), confirm.c_str()) != 0) {
			std::cout << "password err" << std::endl;
			root["error"] = ErrorCodes::PasswdErr;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		//先查找redis中email对应的验证码是否合理
		std::string  varify_code;
		bool b_get_varify = RedisMgr::GetInstance()->Get(CODEPREFIX + src_root["email"].asString(), varify_code);
		//先判断验证码有没有过期
		if (!b_get_varify) {
			std::cout << " get varify code expired" << std::endl;
			root["error"] = ErrorCodes::VarifyExpired;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}
		//再判断验证码是不是verifyserver发给邮箱的服务
		if (varify_code != src_root["varifycode"].asString()) {
			std::cout << " varify code error" << std::endl;
			root["error"] = ErrorCodes::VarifyCodeErr;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		//查找数据库判断用户是否存在
		int uid = MysqlMgr::GetInstance()->RegUser(name, email, pwd, icon);
		if (uid == 0 || uid == -1) {
			std::cout << " user or email exist" << std::endl;
			root["error"] = ErrorCodes::UserExist;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}
		root["error"] = 0;
		root["uid"] = uid;
		root["email"] = email;
		root["user"] = name;
		root["passwd"] = pwd;
		root["confirm"] = confirm;
		root["icon"] = icon;
		root["varifycode"] = src_root["varifycode"].asString();
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;
		return true;
		});

	//重置回调逻辑
	RegPost("/reset_pwd", [](std::shared_ptr<HttpConnection> connection) {
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
		std::cout << "receive body is " << body_str << std::endl;
		connection->_response.set(http::field::content_type, "text/json");
		Json::Value root;
		Json::Reader reader;
		Json::Value src_root;
		bool parse_success = reader.parse(body_str, src_root);
		if (!parse_success) {
			std::cout << "Failed to parse JSON data!" << std::endl;
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		auto email = src_root["email"].asString();
		auto name = src_root["user"].asString();
		auto pwd = src_root["passwd"].asString();

		//先查找redis中email对应的验证码是否合理
		std::string  varify_code;
		bool b_get_varify = RedisMgr::GetInstance()->Get(CODEPREFIX + src_root["email"].asString(), varify_code);
		if (!b_get_varify) {
			std::cout << " get varify code expired" << std::endl;
			root["error"] = ErrorCodes::VarifyExpired;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		if (varify_code != src_root["varifycode"].asString()) {
			std::cout << " varify code error" << std::endl;
			root["error"] = ErrorCodes::VarifyCodeErr;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}
		//查询数据库判断用户名和邮箱是否匹配
		bool email_valid = MysqlMgr::GetInstance()->CheckEmail(name, email);
		if (!email_valid) {
			std::cout << " user email not match" << std::endl;
			root["error"] = ErrorCodes::EmailNotMatch;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		//更新密码为最新密码
		bool b_up = MysqlMgr::GetInstance()->UpdatePwd(name, pwd);
		if (!b_up) {
			std::cout << " update pwd failed" << std::endl;
			root["error"] = ErrorCodes::PasswdUpFailed;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		std::cout << "succeed to update password" << pwd << std::endl;
		root["error"] = 0;
		root["email"] = email;
		root["user"] = name;
		root["passwd"] = pwd;
		root["varifycode"] = src_root["varifycode"].asString();
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;
		return true;
		});

	//用户登录逻辑
	RegPost("/user_login", [](std::shared_ptr<HttpConnection> connection) {
		//从 connection->_request 取出 POST 请求的 body 数据，转换成 std::string
		//这里的 connection 是一个 std::shared_ptr<HttpConnection>，代表当前 HTTP 连接。
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
		std::cout << "receive body is " << body_str << std::endl;//打印出
		connection->_response.set(http::field::content_type, "text/json");//设置响应的格式

		//json解析
		Json::Value root;//响应数据放这里
		Json::Reader reader;
		Json::Value src_root;//源数据防这里面
		bool parse_success = reader.parse(body_str, src_root);
		if (!parse_success) {//解析失败，返回失败的响应
			std::cout << "Failed to parse JSON data!" << std::endl;
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();//将响应的数据反序列化
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}
		//读取用户名(邮箱)和密码
		auto email = src_root["email"].asString();
		auto pwd = src_root["passwd"].asString();
		UserInfo userInfo;
		//查询数据库判断用户名和密码是否匹配，匹配的话就添加到userInfo中
		bool pwd_valid = MysqlMgr::GetInstance()->CheckPwd(email, pwd, userInfo);
		if (!pwd_valid) {
			std::cout << " email pwd not match" << std::endl;
			root["error"] = ErrorCodes::PasswdInvalid;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		//查询StatusServer找到合适的连接(查询token和id)
		auto reply = StatusGrpcClient::GetInstance()->GetChatServer(userInfo.uid);
		//给状态服务器发的是GetChatServer请求
		if (reply.error()) {
			std::cout << " grpc get chat server failed, error is " << reply.error() << std::endl;
			root["error"] = ErrorCodes::RPCGetFailed;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		std::cout << "succeed to load userinfo uid is " << userInfo.uid << std::endl;
		root["error"] = 0;
		root["email"] = email;//src_root["email"]
		root["uid"] = userInfo.uid;
		root["token"] = reply.token();
		root["host"] = reply.host();
		root["port"] = reply.port();
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;
		return true;
		});
	
}

//实现HandleGet处理请求，这样可以在connection中调用这个处理请求
bool LogicSystem::HandleGet(std::string path, std::shared_ptr<HttpConnection> conn) {
	if (_get_handlers.find(path) == _get_handlers.end()) {//哈希表中没有找到，返回错误
		return false;
	}
	_get_handlers[path](conn);//找到了，返回对应的处理器_get_handlers[path]，然后回调需要conn
	return true;
}
//实现Post请求的处理
bool LogicSystem::HandlePost(std::string path, std::shared_ptr<HttpConnection> conn) {
	if (_post_handlers.find(path) == _post_handlers.end()) {
		return false;
	}
	_post_handlers[path](conn);
	return true;
}
