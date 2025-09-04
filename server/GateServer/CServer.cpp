#include <iostream>
#include "CServer.h"
#include "HttpConnection.h"
#include "AsioIOServicePool.h"

//实现构造函数
CServer::CServer(boost::asio::io_context& ioc, unsigned short& port)
	: _ioc(ioc),  _acceptor(ioc, tcp::endpoint(tcp::v4(), port)) {

}

//实现start函数用来监听新的连接
void CServer::Start() {
	auto self = shared_from_this();//
	auto& io_context = AsioIOServicePool::GetInstance()->GetIOService();
	std::shared_ptr<HttpConnection> new_con = std::make_shared<HttpConnection>(io_context);
	_acceptor.async_accept(new_con->GetSocket(), [self, new_con](beast::error_code ec) {
		//由于boost::asio存在出现异常关闭的情况下，进入catch的逻辑中
		//这里使用try/catch规避，如果正常关闭，一定在try中
		try {
			//这个连接出错的话，Start()函数会关闭旧连接，需要重新调用Start()来监听新连接
			if (ec) {
				self->Start();
				return;
			}
			//没出错的话，处理链接，创建HpptConnection类管理连接
			//直接用HttpConnection>(std::move(self->_socket)，那么他可能会随着这个作用域失效
			//创建HttpConnection类型智能指针，将_socket内部数据转移给HttpConnection管理，再用智能指针托管
			// _socket继续用来接受写的链接。
			new_con->Start();
			//继续监听新的连接
			self->Start();
		}
		catch (std::exception& exp) {
			std::cout << "exception is " << exp.what() << std::endl;
			self->Start();
		}
	});

}

