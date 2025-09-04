#pragma once
#include <boost/asio.hpp>
#include "CSession.h"
#include <memory.h>
#include <map>
#include <mutex>
//#include <boost/asio/steady_timer.hpp>
using namespace std;
using boost::asio::ip::tcp;

//服务功能
class CServer
{
public:
	CServer(boost::asio::io_context& io_context, short port);
	~CServer();
	//发现session无效，利用哈希表：string--[map]-->shared_ptr<CSession>，清除掉session
	void ClearSession(std::string);
	////根据uid获取session
	//shared_ptr<CSession> GetSession(std::string);
	//bool CheckValid(std::string);
	//void on_timer(const boost::system::error_code& ec);
	//void StartTimer();
	//void StopTimer();
private:

	//接收连接处理的回调
	void HandleAccept(shared_ptr<CSession>, const boost::system::error_code& error);
	//开始接收连接
	void StartAccept();
	boost::asio::io_context& _io_context;
	short _port;
	tcp::acceptor _acceptor;//boost:asio:下的tcp的accept，接收器
	std::map<std::string, shared_ptr<CSession>> _sessions;
	std::mutex _mutex;
	//boost::asio::steady_timer _timer;
};