#pragma once
#include <boost/beast/http.hpp>
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <memory>
namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

class CServer : public std::enable_shared_from_this<CServer>
{
	//CServer.h中声明acceptor, 以及用于事件循环的上下文iocontext,和构造函数
public:
	//构造函数
	CServer(boost::asio::io_context& ioc, unsigned short& port);//接受一个端口号
	//服务器启动，监听新的连接
	void Start();
private:
	//创建acceptor接受新到来的链接
	tcp::acceptor _acceptor;//需要绑定io_context，这样io_context才会把底层就绪的连接事件抛给它
	net::io_context& _ioc;
	//每有连接来，会有一个socket存储连接信息。另外新连接来了以后旧的连接转给连接类，_socket还可以复用，存储新的连接
};

