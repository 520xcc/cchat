#include "LogicSystem.h"
#include <csignal>
#include <thread>
#include <mutex>
#include "AsioIOServicePool.h"
#include "CServer.h"
#include "ConfigMgr.h"
//#include <iostream>
#include "RedisMgr.h"
#include "ChatServiceImpl.h"
#include "const.h"

using namespace std;
bool bstop = false;
std::condition_variable cond_quit;
std::mutex mutex_quit;



int main()
{   
	//1. 从配置管理器中获取服务器配置（如服务器名、主机地址、端口等）。
	auto& cfg = ConfigMgr::Inst();

	auto server_name = cfg["SelfServer"]["Name"];
	try {
		
		//2. 初始化一个Asio I / O服务池（用于处理异步I / O操作）
		auto pool = AsioIOServicePool::GetInstance();
		//原来错误的情况就是，客户端连接第一个服务器（count等于1），此后客户端关闭后，count被置为0，这时候再次开启客户端连接，还是从第一个服务器开始遍历，还是会连接到第一个
		//3. 在Redis中记录当前服务器的登录人数（初始化为0）
		RedisMgr::GetInstance()->HSet(LOGIN_COUNT, server_name, "0");
		

		//4. 创建并启动一个gRPC服务器线程，用于处理聊天服务（`ChatServiceImpl`）。
		std::string server_address(cfg["SelfServer"]["Host"] + ":" + cfg["SelfServer"]["RPCPort"]);
		ChatServiceImpl service;
		grpc::ServerBuilder builder;
		// 5. 在一个单独的线程中运行gRPC服务器，监听端口和添加服务
		builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
		builder.RegisterService(&service);
		// 构建并启动gRPC服务器
		std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
		std::cout << "RPC Server listening on " << server_address << std::endl;

		//单独启动一个线程处理grpc服务
		std::thread  grpc_server_thread([&server]() {
			server->Wait();
			});

		//6. 设置一个Boost.Asio的信号集，用于捕获SIGINT和SIGTERM信号（如Ctrl + C），以便优雅地关闭服务器。
		boost::asio::io_context  io_context;
		boost::asio::signal_set signals(io_context, SIGINT, SIGTERM, SIGBREAK);
		signals.async_wait([&io_context, pool, &server](auto, auto) {
			io_context.stop();
			pool->Stop();
			server->Shutdown();
			});

		//7. 启动一个TCP服务器（`CServer`）来处理客户端连接。
		auto port_str = cfg["SelfServer"]["Port"];
		CServer s(io_context, atoi(port_str.c_str()));
		io_context.run();

		//8. 当服务器关闭时，清理资源：从Redis中删除登录人数记录、关闭Redis连接、等待gRPC服务器线程结束。
		RedisMgr::GetInstance()->HDel(LOGIN_COUNT, server_name);
		RedisMgr::GetInstance()->Close();
		grpc_server_thread.join();
	}
	catch (std::exception& e) {
		std::cerr << "Exception: " << e.what() << endl;
	}

}

//int main()
//{
//    std::cout << "[Main] Program start." << std::endl;
//
//    try {
//        // 1. 从配置管理器中获取服务器配置
//        auto& cfg = ConfigMgr::Inst();
//        std::cout << "[Main] ConfigMgr loaded successfully." << std::endl;
//
//        auto server_name = cfg.GetValue("SelfServer", "Name");
//        auto host = cfg.GetValue("SelfServer", "Host");
//        auto port_str = cfg.GetValue("SelfServer", "Port");
//        auto rpc_port = cfg.GetValue("SelfServer", "RPCPort");
//
//        std::cout << "[Config] SelfServer.Name=" << server_name << std::endl;
//        std::cout << "[Config] SelfServer.Host=" << host << std::endl;
//        std::cout << "[Config] SelfServer.Port=" << port_str << std::endl;
//        std::cout << "[Config] SelfServer.RPCPort=" << rpc_port << std::endl;
//
//        // 2. 初始化 Asio I/O 服务池
//        std::cout << "[Main] Init AsioIOServicePool..." << std::endl;
//        auto pool = AsioIOServicePool::GetInstance();
//        std::cout << "[Main] AsioIOServicePool init OK." << std::endl;
//
//        // 3. 在 Redis 中记录当前服务器的登录人数
//        std::cout << "[Main] Init Redis, set login_count=0" << std::endl;
//        RedisMgr::GetInstance()->HSet(LOGIN_COUNT, server_name, "0");
//        std::cout << "[Main] Redis init OK." << std::endl;
//
//        // 4. 创建并启动 gRPC 服务器
//        std::string server_address = host + ":" + rpc_port;
//        std::cout << "[Main] Try start gRPC server on " << server_address << std::endl;
//
//        ChatServiceImpl service;
//        grpc::ServerBuilder builder;
//        builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
//        builder.RegisterService(&service);
//
//        std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
//        if (!server) {
//            std::cerr << "[Error] gRPC server failed to start on " << server_address << std::endl;
//            return -1;
//        }
//        std::cout << "[Main] gRPC server listening on " << server_address << std::endl;
//
//        // 单独启动线程运行 gRPC server
//        std::thread grpc_server_thread([&server]() {
//            std::cout << "[Thread] gRPC server thread started, waiting..." << std::endl;
//            server->Wait();
//            std::cout << "[Thread] gRPC server thread exit." << std::endl;
//            });
//
//        // 5. 设置 Boost.Asio 信号捕获
//        std::cout << "[Main] Setting up signal handlers..." << std::endl;
//        boost::asio::io_context io_context;
//        boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
//        signals.async_wait([&io_context, pool, &server](auto, auto) {
//            std::cout << "[Signal] Caught exit signal, shutting down..." << std::endl;
//            io_context.stop();
//            pool->Stop();
//            server->Shutdown();
//            });
//        std::cout << "[Main] Signal handlers ready." << std::endl;
//
//        // 6. 启动 TCP 服务器
//        std::cout << "[Main] Starting TCP server on port " << port_str << std::endl;
//        int port_num = std::stoi(port_str);
//        CServer s(io_context, port_num);
//
//        std::cout << "[Main] Entering io_context.run() ..." << std::endl;
//        io_context.run();
//        std::cout << "[Main] io_context.run() exited." << std::endl;
//
//        // 7. 清理资源
//        std::cout << "[Main] Cleaning up Redis..." << std::endl;
//        RedisMgr::GetInstance()->HDel(LOGIN_COUNT, server_name);
//        RedisMgr::GetInstance()->Close();
//
//        std::cout << "[Main] Waiting gRPC thread join..." << std::endl;
//        grpc_server_thread.join();
//
//        std::cout << "[Main] Program exit normally." << std::endl;
//    }
//    catch (std::exception& e) {
//        std::cerr << "[Exception] " << e.what() << std::endl;
//    }
//
//    return 0;
//}
