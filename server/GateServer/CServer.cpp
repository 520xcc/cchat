#include <iostream>
#include "CServer.h"
#include "HttpConnection.h"
#include "AsioIOServicePool.h"

//ʵ�ֹ��캯��
CServer::CServer(boost::asio::io_context& ioc, unsigned short& port)
	: _ioc(ioc),  _acceptor(ioc, tcp::endpoint(tcp::v4(), port)) {

}

//ʵ��start�������������µ�����
void CServer::Start() {
	auto self = shared_from_this();//
	auto& io_context = AsioIOServicePool::GetInstance()->GetIOService();
	std::shared_ptr<HttpConnection> new_con = std::make_shared<HttpConnection>(io_context);
	_acceptor.async_accept(new_con->GetSocket(), [self, new_con](beast::error_code ec) {
		//����boost::asio���ڳ����쳣�رյ�����£�����catch���߼���
		//����ʹ��try/catch��ܣ���������رգ�һ����try��
		try {
			//������ӳ���Ļ���Start()������رվ����ӣ���Ҫ���µ���Start()������������
			if (ec) {
				self->Start();
				return;
			}
			//û����Ļ����������ӣ�����HpptConnection���������
			//ֱ����HttpConnection>(std::move(self->_socket)����ô�����ܻ��������������ʧЧ
			//����HttpConnection��������ָ�룬��_socket�ڲ�����ת�Ƹ�HttpConnection������������ָ���й�
			// _socket������������д�����ӡ�
			new_con->Start();
			//���������µ�����
			self->Start();
		}
		catch (std::exception& exp) {
			std::cout << "exception is " << exp.what() << std::endl;
			self->Start();
		}
	});

}

