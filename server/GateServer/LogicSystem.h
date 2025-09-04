#include "const.h"

//Ϊ�˷�ֹ�����ã���ǰ������������������Ҫ��.cpp�ļ��л������� 
class HttpConnection;
typedef std::function<void(std::shared_ptr<HttpConnection>)>HttpHandler;//���庯��������
//�������:�����ӵ�std::function<void(std::shared_ptr<HttpConnection>)>��ΪHttpHandler

class LogicSystem :public Singleton<LogicSystem>
{
	friend class Singleton<LogicSystem>;
public:
	~LogicSystem(){};//�������õĻ�ע�͵�����Ȼ�����޷������Ĳ�������
	bool HandleGet(std::string, std::shared_ptr<HttpConnection>); // ����get���� -> ��һ������url��string���ͣ����ڶ�������������ָ��
	bool HandlePost(std::string path, std::shared_ptr<HttpConnection> conn);
	void RegGet(std::string, HttpHandler handler);    //ע��get���� -> ��һ��������url���ڶ���������һ���ص�����������
	void RegPost(std::string url, HttpHandler handler);
private:
	LogicSystem();
	std::map<std::string, HttpHandler> _get_handlers;//���崦��get����ļ��� 
	std::map<std::string, HttpHandler> _post_handlers;//���崦��post����ļ���

};
