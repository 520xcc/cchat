#include "AsioIOServicePool.h"
#include <iostream>
using namespace std;
//构造初始化
AsioIOServicePool::AsioIOServicePool(std::size_t size) :_ioServices(size),
_works(size), _nextIOService(0) {
	for (std::size_t i = 0; i < size; ++i) {
		_works[i] = std::unique_ptr<Work>(new Work(_ioServices[i]));
		//独占指针管理赋给works[i]
	}

	//遍历多个ioservice，创建多个线程，每个线程内部启动ioservice
	//任务交给线程做，让ioioServices，run()起来，由于有work管理，不会因为没有事件退出
	for (std::size_t i = 0; i < _ioServices.size(); ++i) {
		_threads.emplace_back([this, i]() {
			_ioServices[i].run();
			});
	}
}
//析构
AsioIOServicePool::~AsioIOServicePool() {
	std::cout << "AsioIOServicePool destruct" << endl;
}

boost::asio::io_context& AsioIOServicePool::GetIOService() {
	auto& service = _ioServices[_nextIOService++];
	if (_nextIOService == _ioServices.size()) {
		_nextIOService = 0;//再次轮询
	}
	return service;
}

void AsioIOServicePool::Stop() {
	//因为仅仅执行work.reset并不能让iocontext从run的状态中退出
	//当iocontext已经绑定了读或写的监听事件后，还需要手动stop该服务。
	for (auto& work : _works) {
		//把服务先停止
		work->get_io_context().stop();
		work.reset();//传空指针
	}

	//std::thread::join() 的作用
	//join() 会阻塞当前线程（这里是主线程），直到被 join() 的那个线程执行完毕。
	//如果线程还在运行，join() 会一直等。
	//如果线程已经结束，join() 会立即返回。
	for (auto& t : _threads) {
		t.join();
	}
}