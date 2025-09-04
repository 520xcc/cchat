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
	//CServer.h������acceptor, �Լ������¼�ѭ����������iocontext,�͹��캯��
public:
	//���캯��
	CServer(boost::asio::io_context& ioc, unsigned short& port);//����һ���˿ں�
	//�����������������µ�����
	void Start();
private:
	//����acceptor�����µ���������
	tcp::acceptor _acceptor;//��Ҫ��io_context������io_context�Ż�ѵײ�����������¼��׸���
	net::io_context& _ioc;
	//ÿ��������������һ��socket�洢������Ϣ�����������������Ժ�ɵ�����ת�������࣬_socket�����Ը��ã��洢�µ�����
};

