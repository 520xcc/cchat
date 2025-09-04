#include "HttpConnection.h"
#include <iostream>
#include "LogicSystem.h"
#include "VerifyGrpcClient.h"
#include "RedisMgr.h"
#include "const.h"
#include "MysqlMgr.h"
#include <cstring>
#include "StatusGrpcClient.h"

//ʵ��RegGetע��get����
void LogicSystem::RegGet(std::string url, HttpHandler handler) {
	_get_handlers.insert(make_pair(url, handler));//��ֵ�Ե���ʽ����
}
//ʵ��RegPostע��post����
void LogicSystem::RegPost(std::string url, HttpHandler handler) {
	_post_handlers.insert(make_pair(url, handler));
}
//ʵ�ֹ��캯�����ڹ��캯����д
LogicSystem::LogicSystem() {
	//�ڹ��캯����ʵ�־������Ϣע��
	RegGet("/get_test", [](std::shared_ptr<HttpConnection> connection) {
		beast::ostream(connection->_response.body()) << "receive get_test req" << std::endl;	//������ҪHttpConnection.h��LogicSystem.h�໥����
		int i = 0;
		//��������Ĳ���
		for (auto & elem : connection->_get_params) {
			i++;
			beast::ostream(connection->_response.body()) << "param" << i << " key is " << elem.first;
			beast::ostream(connection->_response.body()) << "param" << i << " value is " << elem.second << std::endl;
		}
	});
	//�ڹ��캯������ӻ�ȡ��֤��Ĵ����߼�
	// 
	// �����ͻ��˶� /get_varifycode �� POST ����
	// ���� JSON �����壬��ȡ���е� email �ֶ�
	// Ȼ����ú�˷����� gRPC �ͻ��ˣ���ȡ��֤��
	// �ٹ���һ�� JSON ��Ӧ���ظ��ͻ��ˡ�
	// 
	//ע��һ�� POST ���͵� HTTP ·�ɣ�/get_varifycode ��ע��� URI ·��
	RegPost("/get_varifycode", [](std::shared_ptr<HttpConnection> connection) {
		//��ȡ������תΪstring������� body_str ��
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
		//��ӡ�յ������������ݣ��鿴�������Ƿ���ȷ��
		std::cout << "receive body is " << body_str << std::endl;
		//�ذ����ã���������Ϊ�ı���json
		connection->_response.set(http::field::content_type, "text/json");

		//����
		Json::Value root;    //���ظ��Է���ʱ����Ҫ�����json�ṹ
		Json::Reader reader; //�����л�
		Json::Value src_root;//��Դ�Ľṹ
		//�����Ƿ�����ɹ�
		bool parse_success = reader.parse(body_str, src_root);//ʹ��readerȥ�����յ����������������ŵ�src_root��
		//�ж��Ƿ�ɹ���Ĳ���
		if (!parse_success) {
			std::cout << "Failed to parse JSON data!" << std::endl;
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;//����һ��ʧ�ܵ���Ӧ�������л��õ�json���ӵ�body��
			return true;
		}
		//��������£��ᷢһ��email����
		//�ж�email�����ڵ��߼�
		if (!src_root.isMember("email")) {
			std::cout << "Failed to parse JSON data!" << std::endl;
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;//����һ��ʧ�ܵ���Ӧ�������л��õ�json���ӵ�body��
			return true;
		}
		//key���ڣ��������email
		auto email = src_root["email"].asString();

		//���˻�ȡ��֤��Ŀͻ��ˣ����һ����ȡ��֤��Ĵ����߼�������varify�ķ����
		GetVarifyRsp rsp = VerifyGrpcClient::GetInstance()->GetVarifyCode(email);

		std::cout << "email is " << email << std::endl;
		root["error"] = rsp.error();//0��ʾû�д���
		//���䷵���Է�
		root["email"] = src_root["email"];
		//����ֱ�ӷ�root�ṹ�������ֽ���->תΪstring
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;//���ַ���д��body()��
		return true;
		});

	RegPost("/user_register", [](std::shared_ptr<HttpConnection> connection) {
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
		std::cout << "receive body is " << body_str << std::endl;
		connection->_response.set(http::field::content_type, "text/json");
		Json::Value root;
		Json::Reader reader;
		Json::Value src_root;
		bool parse_success = reader.parse(body_str, src_root);
		if (!parse_success) {
			std::cout << "Failed to parse JSON data!" << std::endl;
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		
		auto email = src_root["email"].asString();
		auto name = src_root["user"].asString();
		auto pwd = src_root["passwd"].asString();
		auto confirm = src_root["confirm"].asString();
		auto icon = src_root["icon"].asString();
		//�����ȷ�����벻һ��Ҳ����
		 
		//if (pwd != confirm) {
		//	std::cout << "password err" << std::endl;
		//	root["error"] = ErrorCodes::PasswdErr;
		//	std::string jsonstr = root.toStyledString();
		//	beast::ostream(connection->_response.body()) << jsonstr;
		//	return true;
		//}
		if (strcmp(pwd.c_str(), confirm.c_str()) != 0) {
			std::cout << "password err" << std::endl;
			root["error"] = ErrorCodes::PasswdErr;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		//�Ȳ���redis��email��Ӧ����֤���Ƿ����
		std::string  varify_code;
		bool b_get_varify = RedisMgr::GetInstance()->Get(CODEPREFIX + src_root["email"].asString(), varify_code);
		//���ж���֤����û�й���
		if (!b_get_varify) {
			std::cout << " get varify code expired" << std::endl;
			root["error"] = ErrorCodes::VarifyExpired;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}
		//���ж���֤���ǲ���verifyserver��������ķ���
		if (varify_code != src_root["varifycode"].asString()) {
			std::cout << " varify code error" << std::endl;
			root["error"] = ErrorCodes::VarifyCodeErr;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		//�������ݿ��ж��û��Ƿ����
		int uid = MysqlMgr::GetInstance()->RegUser(name, email, pwd, icon);
		if (uid == 0 || uid == -1) {
			std::cout << " user or email exist" << std::endl;
			root["error"] = ErrorCodes::UserExist;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}
		root["error"] = 0;
		root["uid"] = uid;
		root["email"] = email;
		root["user"] = name;
		root["passwd"] = pwd;
		root["confirm"] = confirm;
		root["icon"] = icon;
		root["varifycode"] = src_root["varifycode"].asString();
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;
		return true;
		});

	//���ûص��߼�
	RegPost("/reset_pwd", [](std::shared_ptr<HttpConnection> connection) {
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
		std::cout << "receive body is " << body_str << std::endl;
		connection->_response.set(http::field::content_type, "text/json");
		Json::Value root;
		Json::Reader reader;
		Json::Value src_root;
		bool parse_success = reader.parse(body_str, src_root);
		if (!parse_success) {
			std::cout << "Failed to parse JSON data!" << std::endl;
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		auto email = src_root["email"].asString();
		auto name = src_root["user"].asString();
		auto pwd = src_root["passwd"].asString();

		//�Ȳ���redis��email��Ӧ����֤���Ƿ����
		std::string  varify_code;
		bool b_get_varify = RedisMgr::GetInstance()->Get(CODEPREFIX + src_root["email"].asString(), varify_code);
		if (!b_get_varify) {
			std::cout << " get varify code expired" << std::endl;
			root["error"] = ErrorCodes::VarifyExpired;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		if (varify_code != src_root["varifycode"].asString()) {
			std::cout << " varify code error" << std::endl;
			root["error"] = ErrorCodes::VarifyCodeErr;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}
		//��ѯ���ݿ��ж��û����������Ƿ�ƥ��
		bool email_valid = MysqlMgr::GetInstance()->CheckEmail(name, email);
		if (!email_valid) {
			std::cout << " user email not match" << std::endl;
			root["error"] = ErrorCodes::EmailNotMatch;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		//��������Ϊ��������
		bool b_up = MysqlMgr::GetInstance()->UpdatePwd(name, pwd);
		if (!b_up) {
			std::cout << " update pwd failed" << std::endl;
			root["error"] = ErrorCodes::PasswdUpFailed;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		std::cout << "succeed to update password" << pwd << std::endl;
		root["error"] = 0;
		root["email"] = email;
		root["user"] = name;
		root["passwd"] = pwd;
		root["varifycode"] = src_root["varifycode"].asString();
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;
		return true;
		});

	//�û���¼�߼�
	RegPost("/user_login", [](std::shared_ptr<HttpConnection> connection) {
		//�� connection->_request ȡ�� POST ����� body ���ݣ�ת���� std::string
		//����� connection ��һ�� std::shared_ptr<HttpConnection>������ǰ HTTP ���ӡ�
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
		std::cout << "receive body is " << body_str << std::endl;//��ӡ��
		connection->_response.set(http::field::content_type, "text/json");//������Ӧ�ĸ�ʽ

		//json����
		Json::Value root;//��Ӧ���ݷ�����
		Json::Reader reader;
		Json::Value src_root;//Դ���ݷ�������
		bool parse_success = reader.parse(body_str, src_root);
		if (!parse_success) {//����ʧ�ܣ�����ʧ�ܵ���Ӧ
			std::cout << "Failed to parse JSON data!" << std::endl;
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();//����Ӧ�����ݷ����л�
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}
		//��ȡ�û���(����)������
		auto email = src_root["email"].asString();
		auto pwd = src_root["passwd"].asString();
		UserInfo userInfo;
		//��ѯ���ݿ��ж��û����������Ƿ�ƥ�䣬ƥ��Ļ�����ӵ�userInfo��
		bool pwd_valid = MysqlMgr::GetInstance()->CheckPwd(email, pwd, userInfo);
		if (!pwd_valid) {
			std::cout << " email pwd not match" << std::endl;
			root["error"] = ErrorCodes::PasswdInvalid;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		//��ѯStatusServer�ҵ����ʵ�����(��ѯtoken��id)
		auto reply = StatusGrpcClient::GetInstance()->GetChatServer(userInfo.uid);
		//��״̬������������GetChatServer����
		if (reply.error()) {
			std::cout << " grpc get chat server failed, error is " << reply.error() << std::endl;
			root["error"] = ErrorCodes::RPCGetFailed;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		std::cout << "succeed to load userinfo uid is " << userInfo.uid << std::endl;
		root["error"] = 0;
		root["email"] = email;//src_root["email"]
		root["uid"] = userInfo.uid;
		root["token"] = reply.token();
		root["host"] = reply.host();
		root["port"] = reply.port();
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;
		return true;
		});
	
}

//ʵ��HandleGet������������������connection�е��������������
bool LogicSystem::HandleGet(std::string path, std::shared_ptr<HttpConnection> conn) {
	if (_get_handlers.find(path) == _get_handlers.end()) {//��ϣ����û���ҵ������ش���
		return false;
	}
	_get_handlers[path](conn);//�ҵ��ˣ����ض�Ӧ�Ĵ�����_get_handlers[path]��Ȼ��ص���Ҫconn
	return true;
}
//ʵ��Post����Ĵ���
bool LogicSystem::HandlePost(std::string path, std::shared_ptr<HttpConnection> conn) {
	if (_post_handlers.find(path) == _post_handlers.end()) {
		return false;
	}
	_post_handlers[path](conn);
	return true;
}
