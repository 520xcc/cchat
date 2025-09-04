#pragma once
#include "const.h"
#include <thread>
#include <chrono>

//MySqlPool 启动一个后台线程，每隔 60 秒“体检”连接池里的连接（SELECT 1），防止空闲超时；
//业务线程通过 getConnection() 从池子里取连接，用完通过 returnConnection() 归还。
class SqlConnection {
	//每个连接连接到mysql会有一个连接时长，空闲时长（长时间不操作会断掉）
	//有一个线程会不断查询上次操作时间和当前时间
public:
	SqlConnection(sql::Connection* con, int64_t lasttime) :_con(con), _last_oper_time(lasttime){}
	std::unique_ptr<sql::Connection> _con;
	int64_t _last_oper_time;//增加一个超时时间，不能一直占着mysql不用。
};

class MySqlPool {
public:
	MySqlPool(const std::string& url, const std::string& user, const std::string& pass, const std::string& schema, int poolSize)
		: url_(url), user_(user), pass_(pass), schema_(schema), poolSize_(poolSize), b_stop_(false) {
		try {
			for (int i = 0; i < poolSize_; ++i) {
				sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
				//每次获取driver驱动
				/*std::unique_ptr<sql::Connection> con(driver->connect(url_, user_, pass_));*/
				auto* con = driver->connect(url_, user_, pass_);
				con->setSchema(schema_);
				//获取当前时间戳
				auto currentTime = std::chrono::system_clock::now().time_since_epoch();
				//将时间戳转换为秒
				long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();
				pool_.push(std::make_unique<SqlConnection>(con, timestamp));
			}
			//启动线程（while（_b_stop=false）监测）
			_check_thread = std::thread([this]() {
				while (!b_stop_) {
					checkConnection();
					std::this_thread::sleep_for(std::chrono::seconds(60));
				}
				});
			_check_thread.detach();//后台分离方式运行
		}
		catch (sql::SQLException& e) {
			// 处理异常
			std::cout << "mysql pool init failed, error is" << e.what()<< std::endl;
		}
	}

	void checkConnection() {
		//先上锁
		//这里整个 for 循环都在持锁（lock_guard），
		// 而内部有网络 I/O（executeQuery）与重建连接（driver->connect），都可能阻塞很久，
		// 导致其他线程无法 getConnection() / returnConnection()
		std::lock_guard<std::mutex> guard(mutex_);
		//先获取池子大小，由于队列没有办法从begin()到end()遍历，就只能先把1取出来放到5后面，依次类推
		int poolsize = pool_.size();
		//获取当前时间戳
		auto currentTime = std::chrono::system_clock::now().time_since_epoch();
		//将时间戳转换为秒
		long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();
		for (int i = 0; i < poolsize; i++) {
			auto con = std::move(pool_.front());//先将connection给pop出来
			pool_.pop();
			//在右花括号之前一定会执行一下这个defer，里面封装了一个回调函数，用于避免“漏还”
			Defer defer([this, &con]() {
				pool_.push(std::move(con));
				});
			if (timestamp - con->_last_oper_time < 5) {
				continue;
			}
			//时间大于5的时候操作如下：
			try {
				std::unique_ptr<sql::Statement> stmt(con->_con->createStatement());
				stmt->executeQuery("SELECT 1");
				con->_last_oper_time = timestamp;
				//std::cout << "execute timer alive query , cur is" << timestamp << std::endl;
			}
			catch(sql::SQLException& e){
				std::cout << "Error keeping connection alive:" << e.what() << std::endl;
				//重新创建连接并替换旧的连接
				sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
				auto* newcon = driver->connect(url_, user_, pass_);
				newcon->setSchema(schema_);
				con->_con.reset(newcon);
				con->_last_oper_time = timestamp;
			}
		}
	}

	std::unique_ptr<SqlConnection> getConnection() {//获取连接
		std::unique_lock<std::mutex> lock(mutex_);//加锁
		//判断队列是不是空
		cond_.wait(lock, [this] {
			//先看有没有停止
			if (b_stop_) {
				return true;
			}
			//没有停止信号再判断池子是否为空，池子空返回false-->再来访问的线程挂起
			return !pool_.empty(); });
		//池子也不为空就往下走，接着判断有没有停止
		if (b_stop_) {
			return nullptr;
		}
		//取走池子的第一个
		std::unique_ptr<SqlConnection> con(std::move(pool_.front()));
		pool_.pop();
		return con;
	}

	//把连接还回来的逻辑
	void returnConnection(std::unique_ptr<SqlConnection> con) {
		//先加锁
		std::unique_lock<std::mutex> lock(mutex_);
		//判断有没有停止，停止的话直接return
		if (b_stop_) {
			return;
		}
		//如果没有停止放到后面，并且通知其他挂起的线程
		pool_.push(std::move(con));
		cond_.notify_one();
	}
	//关闭的操作就是直接将这个变量置为true，表示停止，然后通知所有线程
	void Close() {
		b_stop_ = true;
		cond_.notify_all();
	}
	//只清了队列，没有确保后台线程已经结束；线程若在之后继续访问 mutex_ 或 pool_，就是未定义行为。

	~MySqlPool() {
		std::unique_lock<std::mutex> lock(mutex_);
		while (!pool_.empty()) {
			pool_.pop();
		}
	}

private:
	std::string url_; //连接mysql的url
	std::string user_;//连接mysql的用户名
	std::string pass_;//连接mysql的密码
	std::string schema_;//连接mysql使用哪个数据库
	int poolSize_;
	std::queue<std::unique_ptr<SqlConnection>> pool_;//队列
	std::mutex mutex_;  //多线程访问队列的安全性
	std::condition_variable cond_;//队列为空，访问的线程挂起（条件变量）
	std::atomic<bool> b_stop_;//池子退出的时候，标记置为true
	std::thread _check_thread; //心跳机制（一分钟检测），用户长期没操作，告诉mysql连接还活着
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
	//后期要写
	bool CheckEmail(const std::string& name, const std::string& email);
	bool UpdatePwd(const std::string& name, const std::string& newpwd);
	bool CheckPwd(const std::string& email, const std::string& pwd, UserInfo& userInfo);
private:
	std::unique_ptr<MySqlPool> pool_;
};
