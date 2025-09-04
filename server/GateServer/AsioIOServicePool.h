#include <vector>
#include <boost/asio.hpp>
#include "Singleton.h"
class AsioIOServicePool :public Singleton<AsioIOServicePool>
{
    friend Singleton<AsioIOServicePool>;
public:
    using IOService = boost::asio::io_context;
    using Work = boost::asio::io_context::work;
    //boost::asio::io_context::work���ã�
    // io_context����io_context.run��ʱ�����io_contextû�а󶨼����¼�����ֱ�ӷ��ز������ȴ�����ѯ
    // ʹ��work��io_context��Ͳ����������ؽ������˳���������ѯ
    using WorkPtr = std::unique_ptr<Work>;
    ~AsioIOServicePool();
    AsioIOServicePool(const AsioIOServicePool&) = delete;
    AsioIOServicePool& operator=(const AsioIOServicePool&) = delete;
    // ʹ�� round-robin �ķ�ʽ����һ�� io_service
    boost::asio::io_context& GetIOService();//����������
    void Stop();//�����ֹͣ����stop��������Դ��֪ͨ�����ѹ�����߳�
private:
    AsioIOServicePool(std::size_t size = 2/*std::thread::hardware_concurrency()���ݺ��������߳�����������2�͹���*/); //�ᴫ��Ĭ�ϴ�С
    std::vector<IOService> _ioServices;//������
    std::vector<WorkPtr> _works;//��ֹ�������˳�
    std::vector<std::thread> _threads;//�ж����������ж����߳�
    std::size_t                        _nextIOService;//������һ������������
};