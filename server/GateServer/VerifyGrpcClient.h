#pragma once

#include "const.h"
#include "Singleton.h"

using grpc::Channel;
using grpc::Status;
using grpc::ClientContext;

using message::GetVarifyReq;
using message::GetVarifyRsp;
using message::VarifyService;

//写一个池子
class RPConPool {
public:
	RPConPool(size_t poolsize, std::string host, std::string port) :
		poolSize_(poolsize), host_(host), port_(port), b_stop_(false) {
		for (size_t i = 0; i < poolSize_; ++i) {
			std::shared_ptr<Channel> channel = grpc::CreateChannel(host + ":" + port,
				grpc::InsecureChannelCredentials());
			connections_.push(VarifyService::NewStub(channel));//走的是拷贝构造还是移动构造
		}
	}
	~RPConPool() {//加锁让这个池子成为互斥资源
		std::lock_guard<std::mutex> lock(mutex_);//加上锁然后通知，能保证关闭服务，新的连接进不来。也可以保证如果有其他的连接占用服务，我也不能析构，等别人用完
		Close();
		while (!connections_.empty()) {
			connections_.pop();
		}
	}
	void Close() {
		b_stop_ = true;
		cond_.notify_all();//关闭之前通知
	}

	//获取一个连接
	std::unique_ptr<VarifyService::Stub> getConnection() {
		//先加锁，然后用条件变量等待（等这个锁）
		std::unique_lock<std::mutex> lock(mutex_);
		//判断的lambda表达式，如果锁返回的是false就解锁，需要把锁传进去
		cond_.wait(lock, [this]() {
			if (b_stop_) {//停止
				return true;
			}
			return !connections_.empty();//没停止，判断队列是不是空
			});

		//如果停止则直接返回空指针
		if (b_stop_) {
			return  nullptr;
		}
		auto context = std::move(connections_.front());
		connections_.pop();
		return context;
	}

	//用完连接放回池子里面，回收给别人用
	void returnConnection(std::unique_ptr<VarifyService::Stub> context) {
		std::lock_guard<std::mutex> lock(mutex_);
		if (b_stop_) {
			return;//说明资源不可用不用放回去也行
		}
		connections_.push(std::move(context));
		cond_.notify_one();
	}

private:
	std::atomic<bool> b_stop_;
	size_t poolSize_;
	std::string host_;
	std::string port_;
	std::queue<std::unique_ptr<VarifyService::Stub>> connections_;//这里可以做优化，一头一尾创建两把锁
	std::condition_variable cond_;
	std::mutex mutex_;
};


class VerifyGrpcClient :public Singleton<VerifyGrpcClient>
{
	friend class Singleton<VerifyGrpcClient>;
public:
	
	//写一个获取验证码的服务，会获取一个回包这个回包就是GetVarifyRsp类型
	GetVarifyRsp GetVarifyCode(std::string email) {
		//需要有客户端的上下文
		ClientContext context;
		GetVarifyRsp reply;
		GetVarifyReq request;

		//请求，设置一下email
		request.set_email(email);

		//请求后会返回一个状态
		//stub可以理解成媒介，只有通过stub才能和外界通信
		auto stub = pool_->getConnection();//从池子获取连接
		Status status = stub->GetVarifyCode(&context, request, &reply);

		if (status.ok()) {//如果成功，返回响应reply
			pool_->returnConnection(std::move(stub));//用完回收
			return reply;
		}
		else {//如果失败，设置一下错误码
			pool_->returnConnection(std::move(stub));//用完回收
			reply.set_error(ErrorCodes::RPCFailed);
			return reply;
		}
	}

private:
	//构造函数
	VerifyGrpcClient();
	std::unique_ptr<RPConPool> pool_;
};