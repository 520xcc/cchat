#include "AsioIOServicePool.h"
#include <iostream>
using namespace std;
AsioIOServicePool::AsioIOServicePool(std::size_t size) :_ioServices(size),
_works(size), _nextIOService(0) {
    //����work��io_context��
    for (std::size_t i = 0; i < size; ++i) {
        _works[i] = std::unique_ptr<Work>(new Work(_ioServices[i]));
    }
    
    //�������ioservice����������̣߳�ÿ���߳��ڲ�����ioservice
    for (std::size_t i = 0; i < _ioServices.size(); ++i) {
        _threads.emplace_back([this, i]() {
            _ioServices[i].run();//���ڴ����߳��������ĵ�ʱ��ʹ������û���¼��ص�Ҳ�����˳�
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
void AsioIOServicePool::Stop() {//����֮ǰ�ã�֪�����Ӳ�Ҫ���ˣ���stopȻ��������ʱ��ͻ���ó��ӵ�����
    //��Ϊ����ִ��work.reset��������iocontext��run��״̬���˳�
    //��iocontext�Ѿ����˶���д�ļ����¼��󣬻���Ҫ�ֶ�stop�÷���
    for (auto& work : _works) {
        //�ѷ�����ֹͣ
        work->get_io_context().stop();//�Ȱ�ԭ����������ֹͣ��������������ע����񲻻�ע�ᵽ����
        work.reset();//Ȼ���Լ���ɿ�ָ�룬����work���ڲ��������ģ�����run���������أ��߳��˳�
    }
    //����Щ�߳��˳��Ƚ�������д�����ܵ����߳�û�л����꿨ס��������̶߳��˳�����stop()
    for (auto& t : _threads) {
        t.join();
    }
}