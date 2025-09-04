#include "ChatGrpcClient.h"
#include "RedisMgr.h"

//���캯��
//��ȡ�����ļ�����ȡ����������б�--��������std::stringstream���������ŷָ��ķ������б�
//Ϊÿ���������������ӳء�--Ȼ��Ϊÿ������������ChatConPool���󣬴洢��һ��ӳ����С�
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

//������AddFriendReq������ȡĿ���û�ID����ֻ�Ƿ�����һ���յ�AddFriendRsp��Ӧ��
AddFriendRsp ChatGrpcClient::NotifyAddFriend(const AddFriendReq& req)
{

	//��Ҫ����
	// 1. �����֣�����Ŀ���û����ڵ����������---// �û������߻�δ���ӵ��κ����������
	// 2. ��ȡ��Ӧ�����������ӳ�
	// 3. �����ӳػ�ȡ stub ������ RPC
	auto to_uid = req.touid();
	std::string  uid_str = std::to_string(to_uid);

	AddFriendRsp rsp;
	return rsp;
}