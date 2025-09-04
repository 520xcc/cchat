#include "HttpConnection.h"
#include "const.h"
#include <iostream>
#include "LogicSystem.h"

//实现构造函数
HttpConnection::HttpConnection(boost::asio::io_context& ioc) //socket没有默认的构造函数拷贝构造只有移动构造
	: _socket(ioc) {

}

//实现HttpConnection的Start函数
void HttpConnection::Start() {
	auto self = shared_from_this();
	http::async_read(_socket, _buffer, _request, [self](beast::error_code ec,
		std::size_t bytes_transferred) {
			try {
				//出错
				if (ec) {//源码：会返回一个bool值。friend bool operator==( const error_code & code, const error_condition & condition ) BOOST_NOEXCEPT;
					std::cout << "http read err is " << ec.what() << std::endl;//出错打印日志
					return;
				}
				//没有错
				boost::ignore_unused(bytes_transferred);//不需要粘包处理，已经发送的数据忽略
				self->HandleReq();  //处理请求（在后面实现）
				self->CheckDeadline();  //返回的话可能超时，启动超时检测
			}
			catch (std::exception& exp) { //出错打印异常
				std::cout << "exception is " << exp.what() << std::endl;
			}
		}
	);
}

//在HttpConnection::Start内部调用http::async_read函数

/*1
	async_read(
	AsyncReadStream& stream,	//异步读写流
	DynamicBuffer& buffer,		//动态缓存
	basic_parser<isRequest>& parser,//解析器，解析头部
	ReadHandler&& handler)		//回调，解析好了会触发回调

第一个参数为异步可读的数据流，可以理解为socket.
第二个参数为一个buffer，用来存储接受的数据，因为http可接受文本，图像，音频等多种资源文件，所以是Dynamic动态类型的buffer。
第三个参数是请求参数，我们一般也要传递能接受多种资源类型的请求参数。
第四个参数为回调函数，接受成功或者失败，都会触发回调函数，我们用lambda表达式就可以了。
我们已经将1, 2, 3这几个参数写到HttpConnection类的成员声明里了

*/

//实现HandleReq
//http请求：
/*
方法(POST) / url(api/login HTTP) /版本(1.1)
Host: example.com
User - Agent : curl / 7.64.1
Accept : xx/xx
Content-Type: application/json
Content-Length: 48

{
  "username": "user123",
  "password": "secret"
}
*/

//十进制的char转为16进制
unsigned char ToHex(unsigned char x)
{
	return  x > 9 ? x + 55 : x + 48;
}

//16进制转为十进制
unsigned char FromHex(unsigned char x)
{
	unsigned char y;
	if (x >= 'A' && x <= 'Z') y = x - 'A' + 10;
	else if (x >= 'a' && x <= 'z') y = x - 'a' + 10;
	else if (x >= '0' && x <= '9') y = x - '0';
	else assert(0);
	return y;
}

//实现url编码
std::string UrlEncode(const std::string& str)
{
	std::string strTemp = "";
	size_t length = str.length();
	for (size_t i = 0; i < length; i++)
	{
		//判断是否仅有数字和字母构成
		if (isalnum((unsigned char)str[i]) ||
			(str[i] == '-') ||
			(str[i] == '_') ||
			(str[i] == '.') ||
			(str[i] == '~'))
			strTemp += str[i];
		else if (str[i] == ' ') //为空字符
			strTemp += "+";
		else
		{
			//其他字符需要提前加%并且高四位和低四位分别转为16进制
			strTemp += '%';
			strTemp += ToHex((unsigned char)str[i] >> 4);
			strTemp += ToHex((unsigned char)str[i] & 0x0F);
		}
	}
	return strTemp;
}
//url解码
std::string UrlDecode(const std::string& str)
{
	std::string strTemp = "";
	size_t length = str.length();
	for (size_t i = 0; i < length; i++)
	{
		//还原+为空
		if (str[i] == '+') strTemp += ' ';
		//遇到%将后面的两个字符从16进制转为char再拼接
		else if (str[i] == '%')
		{
			assert(i + 2 < length);
			unsigned char high = FromHex((unsigned char)str[++i]);
			unsigned char low = FromHex((unsigned char)str[++i]);
			strTemp += high * 16 + low;
		}
		else strTemp += str[i];
	}
	return strTemp;
}
//实现参数解析
void HttpConnection::PreParseGetParam() {
	// 提取 URI get_test?key1=value1&key2=value2 
	auto uri = _request.target();
	// 查找查询字符串的开始位置（即 '?' 的位置）  
	auto query_pos = uri.find('?');
	if (query_pos == std::string::npos) {
		_get_url = uri;
		return;
	}

	_get_url = uri.substr(0, query_pos);
	std::string query_string = uri.substr(query_pos + 1);
	std::string key;
	std::string value;
	size_t pos = 0;
	while ((pos = query_string.find('&')) != std::string::npos) {
		auto pair = query_string.substr(0, pos);
		size_t eq_pos = pair.find('=');
		if (eq_pos != std::string::npos) {
			key = UrlDecode(pair.substr(0, eq_pos)); // 假设有 url_decode 函数来处理URL解码  
			value = UrlDecode(pair.substr(eq_pos + 1));
			_get_params[key] = value;
		}
		query_string.erase(0, pos + 1);
	}
	// 处理最后一个参数对（如果没有 & 分隔符）  
	if (!query_string.empty()) {
		size_t eq_pos = query_string.find('=');
		if (eq_pos != std::string::npos) {
			key = UrlDecode(query_string.substr(0, eq_pos));
			value = UrlDecode(query_string.substr(eq_pos + 1));
			_get_params[key] = value;
		}
	}
}

//处理对方的请求
void HttpConnection::HandleReq() {
	//设置版本
	_response.version(_request.version());
	//设置为短链接
	_response.keep_alive(false);


	//（1）处理get方法的请求
	if (_request.method() == http::verb::get) {
		//get后先解析url
		PreParseGetParam();
		//使用一个单例的逻辑类LogicSystem处理这个请求，请求的url通过target获取到，后面实现
		bool success = LogicSystem::GetInstance()->HandleGet(_get_url, shared_from_this());//LogicSystem（要去.h文件声明友元类）
		if (!success) {
			_response.result(http::status::not_found);//失败的原因
			_response.set(http::field::content_type, "text/plain");//设置响应的头类型（文本类型/二进制类型）
			beast::ostream(_response.body()) << "url not found\r\n";//回包的内容，写数据响应给对方。将"url not found\r\n"写给ostream，ostream再给到body()
			WriteResponse();//响应包的内容
			return;
		}

		_response.result(http::status::ok);
		_response.set(http::field::server, "GateServer");//设置服务器的类型，什么服务回应的消息
		WriteResponse();
		return;
	}
	//（2）处理post方法请求
	if (_request.method() == http::verb::post) {
		bool success = LogicSystem::GetInstance()->HandlePost(_request.target(), shared_from_this());
		if (!success) {
			_response.result(http::status::not_found);
			_response.set(http::field::content_type, "text/plain");
			beast::ostream(_response.body()) << "url not found\r\n";
			WriteResponse();
			return;
		}

		_response.result(http::status::ok);
		_response.set(http::field::server, "GateServer");
		WriteResponse();
		return;
	}
}

//接下来实现WriteResponse()
void HttpConnection::WriteResponse() {
	auto self = shared_from_this();
	//写包格式，包长度
	_response.content_length(_response.body().size());
	//内容写给对方，往socket中写，回应response写给对方，写完（成功、失败）会有一个回调，需要捕获一下
	http::async_write(_socket, _response, [self](beast::error_code ec,
		std::size_t bytes_transferred) {
			//具体处理
			self->_socket.shutdown(tcp::socket::shutdown_send, ec);//关闭服务器方的发送
			self->deadline_.cancel();//定义的定时器里面有一个cancel函数，用来取消定时器，后面实现
		});
}

/*
async_write(
	AsyncWriteStream& stream,  //socket
	message<isRequest, Body, Fields> const& msg,  //响应
	WriteHandler&& handler,  //回调--->查看源码：beast::error_code ec, std::size_t bytes_transferred
	typename std::enable_if<
		! is_mutable_body_writer<Body>::value>::type*)
*/

//接下来实现CheckDeadline()
void HttpConnection::CheckDeadline() {
	auto self = shared_from_this();
	//定时器和socket用法差不多，定时器是io事件、定时器是定时事件
	//异步等待，等完传一个回调
	//上面的异常会关闭服务器，使用deadline_.cancel()，这样就不会调用当前的回调了
	//下面的是长时间没有触发发送回调，也就是客户端一直没有响应，定时器一直检测直到超时强制关闭。
	deadline_.async_wait([self](beast::error_code ec) {
		//没出错
		if (!ec) {
			self->_socket.close(ec);
			}
		});
}