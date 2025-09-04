#pragma once

#include "const.h"
#include "Singleton.h"

using grpc::Channel;
using grpc::Status;
using grpc::ClientContext;

using message::GetVarifyReq;
using message::GetVarifyRsp;
using message::VarifyService;

//дһ������
class RPConPool {
public:
	RPConPool(size_t poolsize, std::string host, std::string port) :
		poolSize_(poolsize), host_(host), port_(port), b_stop_(false) {
		for (size_t i = 0; i < poolSize_; ++i) {
			std::shared_ptr<Channel> channel = grpc::CreateChannel(host + ":" + port,
				grpc::InsecureChannelCredentials());
			connections_.push(VarifyService::NewStub(channel));//�ߵ��ǿ������컹���ƶ�����
		}
	}
	~RPConPool() {//������������ӳ�Ϊ������Դ
		std::lock_guard<std::mutex> lock(mutex_);//������Ȼ��֪ͨ���ܱ�֤�رշ����µ����ӽ�������Ҳ���Ա�֤���������������ռ�÷�����Ҳ�����������ȱ�������
		Close();
		while (!connections_.empty()) {
			connections_.pop();
		}
	}
	void Close() {
		b_stop_ = true;
		cond_.notify_all();//�ر�֮ǰ֪ͨ
	}

	//��ȡһ������
	std::unique_ptr<VarifyService::Stub> getConnection() {
		//�ȼ�����Ȼ�������������ȴ������������
		std::unique_lock<std::mutex> lock(mutex_);
		//�жϵ�lambda���ʽ����������ص���false�ͽ�������Ҫ��������ȥ
		cond_.wait(lock, [this]() {
			if (b_stop_) {//ֹͣ
				return true;
			}
			return !connections_.empty();//ûֹͣ���ж϶����ǲ��ǿ�
			});

		//���ֹͣ��ֱ�ӷ��ؿ�ָ��
		if (b_stop_) {
			return  nullptr;
		}
		auto context = std::move(connections_.front());
		connections_.pop();
		return context;
	}

	//�������ӷŻس������棬���ո�������
	void returnConnection(std::unique_ptr<VarifyService::Stub> context) {
		std::lock_guard<std::mutex> lock(mutex_);
		if (b_stop_) {
			return;//˵����Դ�����ò��÷Ż�ȥҲ��
		}
		connections_.push(std::move(context));
		cond_.notify_one();
	}

private:
	std::atomic<bool> b_stop_;
	size_t poolSize_;
	std::string host_;
	std::string port_;
	std::queue<std::unique_ptr<VarifyService::Stub>> connections_;//����������Ż���һͷһβ����������
	std::condition_variable cond_;
	std::mutex mutex_;
};


class VerifyGrpcClient :public Singleton<VerifyGrpcClient>
{
	friend class Singleton<VerifyGrpcClient>;
public:
	
	//дһ����ȡ��֤��ķ��񣬻��ȡһ���ذ�����ذ�����GetVarifyRsp����
	GetVarifyRsp GetVarifyCode(std::string email) {
		//��Ҫ�пͻ��˵�������
		ClientContext context;
		GetVarifyRsp reply;
		GetVarifyReq request;

		//��������һ��email
		request.set_email(email);

		//�����᷵��һ��״̬
		//stub��������ý�飬ֻ��ͨ��stub���ܺ����ͨ��
		auto stub = pool_->getConnection();//�ӳ��ӻ�ȡ����
		Status status = stub->GetVarifyCode(&context, request, &reply);

		if (status.ok()) {//����ɹ���������Ӧreply
			pool_->returnConnection(std::move(stub));//�������
			return reply;
		}
		else {//���ʧ�ܣ�����һ�´�����
			pool_->returnConnection(std::move(stub));//�������
			reply.set_error(ErrorCodes::RPCFailed);
			return reply;
		}
	}

private:
	//���캯��
	VerifyGrpcClient();
	std::unique_ptr<RPConPool> pool_;
};