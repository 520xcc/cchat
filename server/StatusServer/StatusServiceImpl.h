#pragma once
#include <grpcpp/grpcpp.h>
#include "message.grpc.pb.h"
#include <mutex>

//功能表述：

//1、根据用户 UID 选择一个负载最小的 ChatServer
//2、生成 token 并保存，用于后续用户认证
//3、处理用户登录时的 token 校验

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using message::GetChatServerReq;
using message::GetChatServerRsp;
using message::LoginReq;
using message::LoginRsp;
using message::StatusService;

//聊天服务器：用来保存一个聊天服务器的信息
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
	std::string host;//host：IP 或主机名
	std::string port;//port：端口
	std::string name;//name：服务器名称（在配置文件里定义）
	int con_count;//con_count：当前连接数（用于负载均衡）
};

//继承自StatusService::Service
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
	//_servers：哈希表：保存所有聊天服务器信息（key 是服务器名称）
	
	//first：key（服务器名）
	//second：value（ChatServer 对象）->包含连接数量

	std::mutex _server_mtx;
	//_server_mtx：保护 _servers 读写的互斥锁。

};