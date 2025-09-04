#pragma once
#include <boost/asio.hpp>
#include "CSession.h"
#include <memory.h>
#include <map>
#include <mutex>
//#include <boost/asio/steady_timer.hpp>
using namespace std;
using boost::asio::ip::tcp;

//������
class CServer
{
public:
	CServer(boost::asio::io_context& io_context, short port);
	~CServer();
	//����session��Ч�����ù�ϣ��string--[map]-->shared_ptr<CSession>�������session
	void ClearSession(std::string);
	////����uid��ȡsession
	//shared_ptr<CSession> GetSession(std::string);
	//bool CheckValid(std::string);
	//void on_timer(const boost::system::error_code& ec);
	//void StartTimer();
	//void StopTimer();
private:

	//�������Ӵ���Ļص�
	void HandleAccept(shared_ptr<CSession>, const boost::system::error_code& error);
	//��ʼ��������
	void StartAccept();
	boost::asio::io_context& _io_context;
	short _port;
	tcp::acceptor _acceptor;//boost:asio:�µ�tcp��accept��������
	std::map<std::string, shared_ptr<CSession>> _sessions;
	std::mutex _mutex;
	//boost::asio::steady_timer _timer;
};