#pragma once
#include "const.h"
#include <thread>
#include <chrono>

//MySqlPool ����һ����̨�̣߳�ÿ�� 60 �롰��족���ӳ�������ӣ�SELECT 1������ֹ���г�ʱ��
//ҵ���߳�ͨ�� getConnection() �ӳ�����ȡ���ӣ�����ͨ�� returnConnection() �黹��
class SqlConnection {
	//ÿ���������ӵ�mysql����һ������ʱ��������ʱ������ʱ�䲻������ϵ���
	//��һ���̻߳᲻�ϲ�ѯ�ϴβ���ʱ��͵�ǰʱ��
public:
	SqlConnection(sql::Connection* con, int64_t lasttime) :_con(con), _last_oper_time(lasttime){}
	std::unique_ptr<sql::Connection> _con;
	int64_t _last_oper_time;//����һ����ʱʱ�䣬����һֱռ��mysql���á�
};

class MySqlPool {
public:
	MySqlPool(const std::string& url, const std::string& user, const std::string& pass, const std::string& schema, int poolSize)
		: url_(url), user_(user), pass_(pass), schema_(schema), poolSize_(poolSize), b_stop_(false) {
		try {
			for (int i = 0; i < poolSize_; ++i) {
				sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
				//ÿ�λ�ȡdriver����
				/*std::unique_ptr<sql::Connection> con(driver->connect(url_, user_, pass_));*/
				auto* con = driver->connect(url_, user_, pass_);
				con->setSchema(schema_);
				//��ȡ��ǰʱ���
				auto currentTime = std::chrono::system_clock::now().time_since_epoch();
				//��ʱ���ת��Ϊ��
				long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();
				pool_.push(std::make_unique<SqlConnection>(con, timestamp));
			}
			//�����̣߳�while��_b_stop=false����⣩
			_check_thread = std::thread([this]() {
				while (!b_stop_) {
					checkConnection();
					std::this_thread::sleep_for(std::chrono::seconds(60));
				}
				});
			_check_thread.detach();//��̨���뷽ʽ����
		}
		catch (sql::SQLException& e) {
			// �����쳣
			std::cout << "mysql pool init failed, error is" << e.what()<< std::endl;
		}
	}

	void checkConnection() {
		//������
		//�������� for ѭ�����ڳ�����lock_guard����
		// ���ڲ������� I/O��executeQuery�����ؽ����ӣ�driver->connect���������������ܾã�
		// ���������߳��޷� getConnection() / returnConnection()
		std::lock_guard<std::mutex> guard(mutex_);
		//�Ȼ�ȡ���Ӵ�С�����ڶ���û�а취��begin()��end()��������ֻ���Ȱ�1ȡ�����ŵ�5���棬��������
		int poolsize = pool_.size();
		//��ȡ��ǰʱ���
		auto currentTime = std::chrono::system_clock::now().time_since_epoch();
		//��ʱ���ת��Ϊ��
		long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();
		for (int i = 0; i < poolsize; i++) {
			auto con = std::move(pool_.front());//�Ƚ�connection��pop����
			pool_.pop();
			//���һ�����֮ǰһ����ִ��һ�����defer�������װ��һ���ص����������ڱ��⡰©����
			Defer defer([this, &con]() {
				pool_.push(std::move(con));
				});
			if (timestamp - con->_last_oper_time < 5) {
				continue;
			}
			//ʱ�����5��ʱ��������£�
			try {
				std::unique_ptr<sql::Statement> stmt(con->_con->createStatement());
				stmt->executeQuery("SELECT 1");
				con->_last_oper_time = timestamp;
				//std::cout << "execute timer alive query , cur is" << timestamp << std::endl;
			}
			catch(sql::SQLException& e){
				std::cout << "Error keeping connection alive:" << e.what() << std::endl;
				//���´������Ӳ��滻�ɵ�����
				sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
				auto* newcon = driver->connect(url_, user_, pass_);
				newcon->setSchema(schema_);
				con->_con.reset(newcon);
				con->_last_oper_time = timestamp;
			}
		}
	}

	std::unique_ptr<SqlConnection> getConnection() {//��ȡ����
		std::unique_lock<std::mutex> lock(mutex_);//����
		//�ж϶����ǲ��ǿ�
		cond_.wait(lock, [this] {
			//�ȿ���û��ֹͣ
			if (b_stop_) {
				return true;
			}
			//û��ֹͣ�ź����жϳ����Ƿ�Ϊ�գ����ӿշ���false-->�������ʵ��̹߳���
			return !pool_.empty(); });
		//����Ҳ��Ϊ�վ������ߣ������ж���û��ֹͣ
		if (b_stop_) {
			return nullptr;
		}
		//ȡ�߳��ӵĵ�һ��
		std::unique_ptr<SqlConnection> con(std::move(pool_.front()));
		pool_.pop();
		return con;
	}

	//�����ӻ��������߼�
	void returnConnection(std::unique_ptr<SqlConnection> con) {
		//�ȼ���
		std::unique_lock<std::mutex> lock(mutex_);
		//�ж���û��ֹͣ��ֹͣ�Ļ�ֱ��return
		if (b_stop_) {
			return;
		}
		//���û��ֹͣ�ŵ����棬����֪ͨ����������߳�
		pool_.push(std::move(con));
		cond_.notify_one();
	}
	//�رյĲ�������ֱ�ӽ����������Ϊtrue����ʾֹͣ��Ȼ��֪ͨ�����߳�
	void Close() {
		b_stop_ = true;
		cond_.notify_all();
	}
	//ֻ���˶��У�û��ȷ����̨�߳��Ѿ��������߳�����֮��������� mutex_ �� pool_������δ������Ϊ��

	~MySqlPool() {
		std::unique_lock<std::mutex> lock(mutex_);
		while (!pool_.empty()) {
			pool_.pop();
		}
	}

private:
	std::string url_; //����mysql��url
	std::string user_;//����mysql���û���
	std::string pass_;//����mysql������
	std::string schema_;//����mysqlʹ���ĸ����ݿ�
	int poolSize_;
	std::queue<std::unique_ptr<SqlConnection>> pool_;//����
	std::mutex mutex_;  //���̷߳��ʶ��еİ�ȫ��
	std::condition_variable cond_;//����Ϊ�գ����ʵ��̹߳�������������
	std::atomic<bool> b_stop_;//�����˳���ʱ�򣬱����Ϊtrue
	std::thread _check_thread; //�������ƣ�һ���Ӽ�⣩���û�����û����������mysql���ӻ�����
};

struct UserInfo {
	std::string name;
	std::string pwd;
	int uid;
	std::string email;
};

class MysqlDAO
{
public:
	MysqlDAO();
	~MysqlDAO();
	int RegUser(const std::string& name, const std::string& email, const std::string& pwd);
	int RegUserTransaction(const std::string& name, const std::string& email, const std::string& pwd, const std::string& icon);
	//����Ҫд
	bool CheckEmail(const std::string& name, const std::string& email);
	bool UpdatePwd(const std::string& name, const std::string& newpwd);
	bool CheckPwd(const std::string& email, const std::string& pwd, UserInfo& userInfo);
private:
	std::unique_ptr<MySqlPool> pool_;
};
