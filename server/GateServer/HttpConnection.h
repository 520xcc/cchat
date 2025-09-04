#pragma once
#include "const.h"

class HttpConnection:public std::enable_shared_from_this<HttpConnection>
{
	friend class LogicSystem;
public:
	HttpConnection(boost::asio::io_context& ioc);
	void Start();
	tcp::socket& GetSocket() {
		return _socket;
	}
private:
	void CheckDeadline();
	void WriteResponse();
	void HandleReq();
	void PreParseGetParam();

	tcp::socket _socket;

	//buffer用来接收数据
	beast::flat_buffer _buffer{ 8192 };

	//request请求消息
	http::request<http::dynamic_body> _request;

	//response响应信息
	http::response<http::dynamic_body> _response;

	//deadline用来做定时器判断请求是否超时
	net::steady_timer deadline_{
		_socket.get_executor(), std::chrono::seconds(60)//C++11定义的初始化列表形式用value{初始化xxx}表示
	};
	
	std::string _get_url;
	std::unordered_map<std::string, std::string> _get_params;

};


