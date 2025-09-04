#pragma once
#include <grpcpp/grpcpp.h>
#include "message.grpc.pb.h"
#include <mutex>

//���ܱ�����

//1�������û� UID ѡ��һ��������С�� ChatServer
//2������ token �����棬���ں����û���֤
//3�������û���¼ʱ�� token У��

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using message::GetChatServerReq;
using message::GetChatServerRsp;
using message::LoginReq;
using message::LoginRsp;
using message::StatusService;

//�������������������һ���������������Ϣ
class  ChatServer {
public:
	ChatServer() :host(""), port(""), name(""), con_count(0) {}
	ChatServer(const ChatServer& cs) :host(cs.host), port(cs.port), name(cs.name), con_count(cs.con_count) {}
	ChatServer& operator=(const ChatServer& cs) {
		if (&cs == this) {
			return *this;
		}

		host = cs.host;
		name = cs.name;
		port = cs.port;
		con_count = cs.con_count;
		return *this;
	}
	std::string host;//host��IP ��������
	std::string port;//port���˿�
	std::string name;//name�����������ƣ��������ļ��ﶨ�壩
	int con_count;//con_count����ǰ�����������ڸ��ؾ��⣩
};

//�̳���StatusService::Service
class StatusServiceImpl final : public StatusService::Service
{
public:
	StatusServiceImpl();
	Status GetChatServer(ServerContext* context, const GetChatServerReq* request,
		GetChatServerRsp* reply) override;
	Status Login(ServerContext* context, const LoginReq* request,
		LoginRsp* reply) override;
private:
	void insertToken(int uid, std::string token);
	ChatServer getChatServer();
	std::unordered_map<std::string, ChatServer> _servers;
	//_servers����ϣ���������������������Ϣ��key �Ƿ��������ƣ�
	
	//first��key������������
	//second��value��ChatServer ����->������������

	std::mutex _server_mtx;
	//_server_mtx������ _servers ��д�Ļ�������

};