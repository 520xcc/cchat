#include "HttpConnection.h"
#include "const.h"
#include <iostream>
#include "LogicSystem.h"

//ʵ�ֹ��캯��
HttpConnection::HttpConnection(boost::asio::io_context& ioc) //socketû��Ĭ�ϵĹ��캯����������ֻ���ƶ�����
	: _socket(ioc) {

}

//ʵ��HttpConnection��Start����
void HttpConnection::Start() {
	auto self = shared_from_this();
	http::async_read(_socket, _buffer, _request, [self](beast::error_code ec,
		std::size_t bytes_transferred) {
			try {
				//����
				if (ec) {//Դ�룺�᷵��һ��boolֵ��friend bool operator==( const error_code & code, const error_condition & condition ) BOOST_NOEXCEPT;
					std::cout << "http read err is " << ec.what() << std::endl;//�����ӡ��־
					return;
				}
				//û�д�
				boost::ignore_unused(bytes_transferred);//����Ҫճ�������Ѿ����͵����ݺ���
				self->HandleReq();  //���������ں���ʵ�֣�
				self->CheckDeadline();  //���صĻ����ܳ�ʱ��������ʱ���
			}
			catch (std::exception& exp) { //�����ӡ�쳣
				std::cout << "exception is " << exp.what() << std::endl;
			}
		}
	);
}

//��HttpConnection::Start�ڲ�����http::async_read����

/*1
	async_read(
	AsyncReadStream& stream,	//�첽��д��
	DynamicBuffer& buffer,		//��̬����
	basic_parser<isRequest>& parser,//������������ͷ��
	ReadHandler&& handler)		//�ص����������˻ᴥ���ص�

��һ������Ϊ�첽�ɶ������������������Ϊsocket.
�ڶ�������Ϊһ��buffer�������洢���ܵ����ݣ���Ϊhttp�ɽ����ı���ͼ����Ƶ�ȶ�����Դ�ļ���������Dynamic��̬���͵�buffer��
�������������������������һ��ҲҪ�����ܽ��ܶ�����Դ���͵����������
���ĸ�����Ϊ�ص����������ܳɹ�����ʧ�ܣ����ᴥ���ص�������������lambda���ʽ�Ϳ����ˡ�
�����Ѿ���1, 2, 3�⼸������д��HttpConnection��ĳ�Ա��������

*/

//ʵ��HandleReq
//http����
/*
����(POST) / url(api/login HTTP) /�汾(1.1)
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

//ʮ���Ƶ�charתΪ16����
unsigned char ToHex(unsigned char x)
{
	return  x > 9 ? x + 55 : x + 48;
}

//16����תΪʮ����
unsigned char FromHex(unsigned char x)
{
	unsigned char y;
	if (x >= 'A' && x <= 'Z') y = x - 'A' + 10;
	else if (x >= 'a' && x <= 'z') y = x - 'a' + 10;
	else if (x >= '0' && x <= '9') y = x - '0';
	else assert(0);
	return y;
}

//ʵ��url����
std::string UrlEncode(const std::string& str)
{
	std::string strTemp = "";
	size_t length = str.length();
	for (size_t i = 0; i < length; i++)
	{
		//�ж��Ƿ�������ֺ���ĸ����
		if (isalnum((unsigned char)str[i]) ||
			(str[i] == '-') ||
			(str[i] == '_') ||
			(str[i] == '.') ||
			(str[i] == '~'))
			strTemp += str[i];
		else if (str[i] == ' ') //Ϊ���ַ�
			strTemp += "+";
		else
		{
			//�����ַ���Ҫ��ǰ��%���Ҹ���λ�͵���λ�ֱ�תΪ16����
			strTemp += '%';
			strTemp += ToHex((unsigned char)str[i] >> 4);
			strTemp += ToHex((unsigned char)str[i] & 0x0F);
		}
	}
	return strTemp;
}
//url����
std::string UrlDecode(const std::string& str)
{
	std::string strTemp = "";
	size_t length = str.length();
	for (size_t i = 0; i < length; i++)
	{
		//��ԭ+Ϊ��
		if (str[i] == '+') strTemp += ' ';
		//����%������������ַ���16����תΪchar��ƴ��
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
//ʵ�ֲ�������
void HttpConnection::PreParseGetParam() {
	// ��ȡ URI get_test?key1=value1&key2=value2 
	auto uri = _request.target();
	// ���Ҳ�ѯ�ַ����Ŀ�ʼλ�ã��� '?' ��λ�ã�  
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
			key = UrlDecode(pair.substr(0, eq_pos)); // ������ url_decode ����������URL����  
			value = UrlDecode(pair.substr(eq_pos + 1));
			_get_params[key] = value;
		}
		query_string.erase(0, pos + 1);
	}
	// �������һ�������ԣ����û�� & �ָ�����  
	if (!query_string.empty()) {
		size_t eq_pos = query_string.find('=');
		if (eq_pos != std::string::npos) {
			key = UrlDecode(query_string.substr(0, eq_pos));
			value = UrlDecode(query_string.substr(eq_pos + 1));
			_get_params[key] = value;
		}
	}
}

//����Է�������
void HttpConnection::HandleReq() {
	//���ð汾
	_response.version(_request.version());
	//����Ϊ������
	_response.keep_alive(false);


	//��1������get����������
	if (_request.method() == http::verb::get) {
		//get���Ƚ���url
		PreParseGetParam();
		//ʹ��һ���������߼���LogicSystem����������������urlͨ��target��ȡ��������ʵ��
		bool success = LogicSystem::GetInstance()->HandleGet(_get_url, shared_from_this());//LogicSystem��Ҫȥ.h�ļ�������Ԫ�ࣩ
		if (!success) {
			_response.result(http::status::not_found);//ʧ�ܵ�ԭ��
			_response.set(http::field::content_type, "text/plain");//������Ӧ��ͷ���ͣ��ı�����/���������ͣ�
			beast::ostream(_response.body()) << "url not found\r\n";//�ذ������ݣ�д������Ӧ���Է�����"url not found\r\n"д��ostream��ostream�ٸ���body()
			WriteResponse();//��Ӧ��������
			return;
		}

		_response.result(http::status::ok);
		_response.set(http::field::server, "GateServer");//���÷����������ͣ�ʲô�����Ӧ����Ϣ
		WriteResponse();
		return;
	}
	//��2������post��������
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

//������ʵ��WriteResponse()
void HttpConnection::WriteResponse() {
	auto self = shared_from_this();
	//д����ʽ��������
	_response.content_length(_response.body().size());
	//����д���Է�����socket��д����Ӧresponseд���Է���д�꣨�ɹ���ʧ�ܣ�����һ���ص�����Ҫ����һ��
	http::async_write(_socket, _response, [self](beast::error_code ec,
		std::size_t bytes_transferred) {
			//���崦��
			self->_socket.shutdown(tcp::socket::shutdown_send, ec);//�رշ��������ķ���
			self->deadline_.cancel();//����Ķ�ʱ��������һ��cancel����������ȡ����ʱ��������ʵ��
		});
}

/*
async_write(
	AsyncWriteStream& stream,  //socket
	message<isRequest, Body, Fields> const& msg,  //��Ӧ
	WriteHandler&& handler,  //�ص�--->�鿴Դ�룺beast::error_code ec, std::size_t bytes_transferred
	typename std::enable_if<
		! is_mutable_body_writer<Body>::value>::type*)
*/

//������ʵ��CheckDeadline()
void HttpConnection::CheckDeadline() {
	auto self = shared_from_this();
	//��ʱ����socket�÷���࣬��ʱ����io�¼�����ʱ���Ƕ�ʱ�¼�
	//�첽�ȴ������괫һ���ص�
	//������쳣��رշ�������ʹ��deadline_.cancel()�������Ͳ�����õ�ǰ�Ļص���
	//������ǳ�ʱ��û�д������ͻص���Ҳ���ǿͻ���һֱû����Ӧ����ʱ��һֱ���ֱ����ʱǿ�ƹرա�
	deadline_.async_wait([self](beast::error_code ec) {
		//û����
		if (!ec) {
			self->_socket.close(ec);
			}
		});
}