#include "StatusServiceImpl.h"
#include "ConfigMgr.h"
#include "const.h"
#include "RedisMgr.h"
#include <climits>

//StatusServiceImpl 类是一个 gRPC 服务实现类，主要承担负载均衡和用户认证两大核心功能，用于协调多个聊天服务器（ChatServer）

std::string generate_unique_string() {
	// 创建UUID对象
	boost::uuids::uuid uuid = boost::uuids::random_generator()();

	// 将UUID转换为字符串
	std::string unique_string = to_string(uuid);

	return unique_string;
}

//客户端调用这个 RPC，就会得到：
//		服务器地址（host、port）
//		一个新生成的 token（用于后续验证）
//	并调用 insertToken 存储 token。
Status StatusServiceImpl::GetChatServer(ServerContext* context, const GetChatServerReq* request, GetChatServerRsp* reply)
{
	std::string prefix("c_ status server has received :  ");
	const auto& server = getChatServer();//调用子接口
	reply->set_host(server.host);
	reply->set_port(server.port);
	reply->set_error(ErrorCodes::Success);
	reply->set_token(generate_unique_string());
	insertToken(request->uid(), reply->token());//StatusServer生成token，实现用户认证和负载均衡；ChatServer专注于处理聊天消息
	return Status::OK;
}

StatusServiceImpl::StatusServiceImpl()
{
	//从配置文件读取所有 ChatServer 的信息，并存到 _servers 容器。
	auto& cfg = ConfigMgr::Inst();
	auto server_list = cfg["chatservers"]["Name"];

	std::vector<std::string> words;

	std::stringstream ss(server_list);
	std::string word;

	while (std::getline(ss, word, ',')) {
		words.push_back(word);
	}

	for (auto& word : words) {
		if (cfg[word]["Name"].empty()) {
			continue;
		}

		ChatServer server;//构成chatserver结构后生成一个这样的类对象加入_server
		server.port = cfg[word]["Port"];
		server.host = cfg[word]["Host"];
		server.name = cfg[word]["Name"];
		_servers[server.name] = server;
	}
}

// 选出一个连接数最少的 ChatServer，生成 token，
// 保存 token → uid 的映射，并返回给客户端。
ChatServer StatusServiceImpl::getChatServer() {
	//先随便选一个服务器（_servers.begin()->second）。去Redis查它的连接数（用 LOGIN_COUNT 哈希表，字段是 server.name）。
	//如果 Redis 里没有这个字段，就说明不知道它的负载情况 → 设置为 INT_MAX（表示负载极大，不优先选它）。否则把 Redis 中的字符串转成整数，存入 minServer.con_count。
	std::lock_guard<std::mutex> guard(_server_mtx);//加锁
	auto minServer = _servers.begin()->second;//指向哈希表首个元素的value
	auto count_str = RedisMgr::GetInstance()->HGet(LOGIN_COUNT, minServer.name);
	if (count_str.empty()) {
		//不存在则默认设置为最大
		minServer.con_count = INT_MAX;
	}
	else {
		minServer.con_count = std::stoi(count_str);
	}
	//遍历 _servers 容器中的所有服务器。逐个从 Redis 查询其连接数。如果查不到，设为 INT_MAX。
	//找到连接数更小的服务器时，更新 minServer。

	// 使用范围基于for循环
	for (auto& server : _servers) {

		if (server.second.name == minServer.name) {
			continue;
		}

		auto count_str = RedisMgr::GetInstance()->HGet(LOGIN_COUNT, server.second.name);
		if (count_str.empty()) {
			server.second.con_count = INT_MAX;
		}
		else {
			server.second.con_count = std::stoi(count_str);
		}

		if (server.second.con_count < minServer.con_count) {
			minServer = server.second;
		}
	}

	return minServer;
}

//校验 uid 与 token 是否匹配。
Status StatusServiceImpl::Login(ServerContext* context, const LoginReq* request, LoginRsp* reply)
{
	auto uid = request->uid();
	auto token = request->token();

	//token不存在内存中，从redis中查询，通过构造key-->value
	std::string uid_str = std::to_string(uid);
	std::string token_key = USERTOKENPREFIX + uid_str;
	std::string token_value = "";
	bool success = RedisMgr::GetInstance()->Get(token_key, token_value);
	if (!success) {
		reply->set_error(ErrorCodes::UidInvalid);
		return Status::OK;
	}

	if (token_value != token) {
		reply->set_error(ErrorCodes::TokenInvalid);
		return Status::OK;
	}

	reply->set_error(ErrorCodes::Success);
	reply->set_uid(uid);
	reply->set_token(token);
	return Status::OK;
}
//token的存储方式:（键值对）将生成的 token 存入 _token 容器，往redis中插
void StatusServiceImpl::insertToken(int uid, std::string token)
{
	std::string uid_str = std::to_string(uid);
	std::string token_key = USERTOKENPREFIX + uid_str;
	RedisMgr::GetInstance()->Set(token_key, token);
}