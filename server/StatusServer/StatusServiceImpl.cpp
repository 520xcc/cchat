#include "StatusServiceImpl.h"
#include "ConfigMgr.h"
#include "const.h"
#include "RedisMgr.h"
#include <climits>

//StatusServiceImpl ����һ�� gRPC ����ʵ���࣬��Ҫ�е����ؾ�����û���֤������Ĺ��ܣ�����Э����������������ChatServer��

std::string generate_unique_string() {
	// ����UUID����
	boost::uuids::uuid uuid = boost::uuids::random_generator()();

	// ��UUIDת��Ϊ�ַ���
	std::string unique_string = to_string(uuid);

	return unique_string;
}

//�ͻ��˵������ RPC���ͻ�õ���
//		��������ַ��host��port��
//		һ�������ɵ� token�����ں�����֤��
//	������ insertToken �洢 token��
Status StatusServiceImpl::GetChatServer(ServerContext* context, const GetChatServerReq* request, GetChatServerRsp* reply)
{
	std::string prefix("c_ status server has received :  ");
	const auto& server = getChatServer();//�����ӽӿ�
	reply->set_host(server.host);
	reply->set_port(server.port);
	reply->set_error(ErrorCodes::Success);
	reply->set_token(generate_unique_string());
	insertToken(request->uid(), reply->token());//StatusServer����token��ʵ���û���֤�͸��ؾ��⣻ChatServerרע�ڴ���������Ϣ
	return Status::OK;
}

StatusServiceImpl::StatusServiceImpl()
{
	//�������ļ���ȡ���� ChatServer ����Ϣ�����浽 _servers ������
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

		ChatServer server;//����chatserver�ṹ������һ����������������_server
		server.port = cfg[word]["Port"];
		server.host = cfg[word]["Host"];
		server.name = cfg[word]["Name"];
		_servers[server.name] = server;
	}
}

// ѡ��һ�����������ٵ� ChatServer������ token��
// ���� token �� uid ��ӳ�䣬�����ظ��ͻ��ˡ�
ChatServer StatusServiceImpl::getChatServer() {
	//�����ѡһ����������_servers.begin()->second����ȥRedis���������������� LOGIN_COUNT ��ϣ���ֶ��� server.name����
	//��� Redis ��û������ֶΣ���˵����֪�����ĸ������ �� ����Ϊ INT_MAX����ʾ���ؼ��󣬲�����ѡ����������� Redis �е��ַ���ת������������ minServer.con_count��
	std::lock_guard<std::mutex> guard(_server_mtx);//����
	auto minServer = _servers.begin()->second;//ָ���ϣ���׸�Ԫ�ص�value
	auto count_str = RedisMgr::GetInstance()->HGet(LOGIN_COUNT, minServer.name);
	if (count_str.empty()) {
		//��������Ĭ������Ϊ���
		minServer.con_count = INT_MAX;
	}
	else {
		minServer.con_count = std::stoi(count_str);
	}
	//���� _servers �����е����з������������ Redis ��ѯ��������������鲻������Ϊ INT_MAX��
	//�ҵ���������С�ķ�����ʱ������ minServer��

	// ʹ�÷�Χ����forѭ��
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

//У�� uid �� token �Ƿ�ƥ�䡣
Status StatusServiceImpl::Login(ServerContext* context, const LoginReq* request, LoginRsp* reply)
{
	auto uid = request->uid();
	auto token = request->token();

	//token�������ڴ��У���redis�в�ѯ��ͨ������key-->value
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
//token�Ĵ洢��ʽ:����ֵ�ԣ������ɵ� token ���� _token ��������redis�в�
void StatusServiceImpl::insertToken(int uid, std::string token)
{
	std::string uid_str = std::to_string(uid);
	std::string token_key = USERTOKENPREFIX + uid_str;
	RedisMgr::GetInstance()->Set(token_key, token);
}