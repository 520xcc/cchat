#include "const.h"

//为了防止互引用，提前声明，并且这两个需要在.cpp文件中互相声明 
class HttpConnection;
typedef std::function<void(std::shared_ptr<HttpConnection>)>HttpHandler;//定义函数处理器
//定义别名:将复杂的std::function<void(std::shared_ptr<HttpConnection>)>简化为HttpHandler

class LogicSystem :public Singleton<LogicSystem>
{
	friend class Singleton<LogicSystem>;
public:
	~LogicSystem(){};//析构不用的话注释掉，不然出现无法解析的参数类型
	bool HandleGet(std::string, std::shared_ptr<HttpConnection>); // 处理get请求 -> 第一个参数url（string类型），第二个参数是智能指针
	bool HandlePost(std::string path, std::shared_ptr<HttpConnection> conn);
	void RegGet(std::string, HttpHandler handler);    //注册get请求 -> 第一个参数是url，第二个参数是一个回调（处理器）
	void RegPost(std::string url, HttpHandler handler);
private:
	LogicSystem();
	std::map<std::string, HttpHandler> _get_handlers;//定义处理get请求的集合 
	std::map<std::string, HttpHandler> _post_handlers;//定义处理post请求的集合

};
