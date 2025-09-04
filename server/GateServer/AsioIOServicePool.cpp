#include "AsioIOServicePool.h"
#include <iostream>
using namespace std;
AsioIOServicePool::AsioIOServicePool(std::size_t size) :_ioServices(size),
_works(size), _nextIOService(0) {
    //先让work和io_context绑定
    for (std::size_t i = 0; i < size; ++i) {
        _works[i] = std::unique_ptr<Work>(new Work(_ioServices[i]));
    }
    
    //遍历多个ioservice，创建多个线程，每个线程内部启动ioservice
    for (std::size_t i = 0; i < _ioServices.size(); ++i) {
        _threads.emplace_back([this, i]() {
            _ioServices[i].run();//现在创建线程跑上下文的时候即使上下文没有事件回调也不会退出
            });
    }
}
AsioIOServicePool::~AsioIOServicePool() {
    Stop();
    std::cout << "AsioIOServicePool destruct" << endl;
}
boost::asio::io_context& AsioIOServicePool::GetIOService() {
    auto& service = _ioServices[_nextIOService++];
    if (_nextIOService == _ioServices.size()) {
        _nextIOService = 0;
    }
    return service;
}
void AsioIOServicePool::Stop() {//析构之前用，知道池子不要用了，先stop然后析构的时候就会调用池子的析构
    //因为仅仅执行work.reset并不能让iocontext从run的状态中退出
    //当iocontext已经绑定了读或写的监听事件后，还需要手动stop该服务。
    for (auto& work : _works) {
        //把服务先停止
        work->get_io_context().stop();//先把原来的上下文停止服务，这样其他人注册服务不会注册到这里
        work.reset();//然后自己变成空指针，回收work和内部的上下文，这样run会立即返回，线程退出
    }
    //但有些线程退出比较慢，不写这句可能导致线程没有回收完卡住，必须等线程都退出，才stop()
    for (auto& t : _threads) {
        t.join();
    }
}