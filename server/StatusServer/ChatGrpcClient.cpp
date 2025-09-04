#include "ChatGrpcClient.h"
#include "RedisMgr.h"

//构造函数
//读取配置文件，获取聊天服务器列表--这里用了std::stringstream来解析逗号分隔的服务器列表
//为每个服务器创建连接池。--然后为每个服务器创建ChatConPool对象，存储在一个映射表中。
ChatGrpcClient::ChatGrpcClient()
{
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

		_pools[cfg[word]["Name"]] = std::make_unique<ChatConPool>(5, cfg[word]["Host"], cfg[word]["Port"]);
	}

}

//它接收AddFriendReq请求，提取目标用户ID，但只是返回了一个空的AddFriendRsp响应。
AddFriendRsp ChatGrpcClient::NotifyAddFriend(const AddFriendReq& req)
{

	//需要完善
	// 1. 服务发现：查找目标用户所在的聊天服务器---// 用户不在线或未连接到任何聊天服务器
	// 2. 获取对应服务器的连接池
	// 3. 从连接池获取 stub 并调用 RPC
	auto to_uid = req.touid();
	std::string  uid_str = std::to_string(to_uid);

	AddFriendRsp rsp;
	return rsp;
}