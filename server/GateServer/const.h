#pragma once
#include <boost/beast/http.hpp>
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <memory>
#include "singleton.h"
#include <functional>
#include <map>
#include <unordered_map>
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include <fstream>
#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>
#include "hiredis.h"
#include <cassert>
#include <iostream>
#include <jdbc/mysql_driver.h>
#include <jdbc/mysql_connection.h>
#include <jdbc/cppconn/prepared_statement.h>
#include <jdbc/cppconn/resultset.h>
#include <jdbc/cppconn/statement.h>
#include <jdbc/cppconn/exception.h>
#include <grpcpp/grpcpp.h>
#include "message.grpc.pb.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>


namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

enum ErrorCodes {
	Success = 0,
	Error_Json = 1001,  //Json解析错误
	RPCFailed = 1002,  //RPC请求错误
	VarifyExpired  = 1003, //验证码过期
	VarifyCodeErr = 1004, //验证码有误
	UserExist = 1005,   //用户已经存在
	PasswdErr = 1006,   // 密码错误
	EmailNotMatch = 1007, //邮箱不四配
	PasswdUpFailed = 1008, //更新密码失败
	PasswdInvalid = 1009,  //密码更新失败
	RPCGetFailed = 1010, //获取rpc请求失败
};

//Defer类
class Defer {
public:
	Defer(std::function<void()> func) : func_(func){}
	~Defer() {
		func_();
	}
private:
	std::function<void()> func_;
};
#define CODEPREFIX "code_"