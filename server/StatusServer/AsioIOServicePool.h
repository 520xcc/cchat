#include <vector>
#include <boost/asio.hpp>
#include "Singleton.h"
class AsioIOServicePool :public Singleton<AsioIOServicePool>
{
    friend Singleton<AsioIOServicePool>;
public:
    using IOService = boost::asio::io_context;
    using Work = boost::asio::io_context::work;
    //boost::asio::io_context::work作用：
    // io_context调用io_context.run的时候，如果io_context没有绑定监听事件，会直接返回不阻塞等待或轮询
    // 使用work绑定io_context后就不会立即返回结束（退出），会轮询
    using WorkPtr = std::unique_ptr<Work>;
    ~AsioIOServicePool();
    AsioIOServicePool(const AsioIOServicePool&) = delete;
    AsioIOServicePool& operator=(const AsioIOServicePool&) = delete;
    // 使用 round-robin 的方式返回一个 io_service
    boost::asio::io_context& GetIOService();//返回上下文
    void Stop();//服务池停止调用stop，回收资源，通知并唤醒挂起的线程
private:
    AsioIOServicePool(std::size_t size = 2/*std::thread::hardware_concurrency()根据核数创建线程数，但这里2就够了*/); //会传递默认大小
    std::vector<IOService> _ioServices;//上下文
    std::vector<WorkPtr> _works;//防止上下文退出
    std::vector<std::thread> _threads;//有多少上下文有多少线程
    std::size_t                        _nextIOService;//返回下一个上下文索引
};