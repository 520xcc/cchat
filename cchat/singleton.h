#ifndef SINGLETON_H
#define SINGLETON_H
#include "global.h"

//C++11的单例类实现方式
template <typename T>
class Singleton{
    //单例类的构造函数是不允许被外界构造，但是希望子类继承的时候可以通过子类构造基类
protected:
    Singleton() = default;
    //由于是单例类，拷贝构造和拷贝赋值要设置为delete，不允许拷贝
    Singleton(const Singleton<T>&) = delete;
    Singleton& operator=(const Singleton<T>& st) = delete;
    //这个单例类会返回一个实例，不希望手动回收，使用智能指针（共享指针）管理内存。为什么用静态，希望所有的Singleton返回一个实例。
    static std::shared_ptr<T> _instance;
    //static 表示这个 _instance 是全局唯一的，每个类型 T 拥有自己的 _instance

public:
    // 获取单例实例（线程安全）
    static std::shared_ptr<T> GetInstance(){
        static std::once_flag s_flag;  //static静态局部变量的特性：确保初始化操作只执行一次。
        std::call_once(s_flag, [&](){
            _instance = std::shared_ptr<T>(new T);
        });  //用于实现线程安全的懒汉式初始化。第二个参数是我们要执行的回调函数。
        //也就是第一次调用的时候s_flag内部标记还是false(表示未初始化)，初始化调用call_once之后就变为true。那么第二次调用GetInstance()时就不会再走初始化操作了。
        //为什么这里使用的时new T不使用make_shared来创建托管？ 因为make_shared需要调用Singleton()构造函数，而这里的托管对象是单例，构造函数私有了

        return _instance;
    }
    void PrintAddress() {
            std::cout << _instance.get() << std::endl; //打印智能指针裸指针地址
    }
    //调用析构函数的时候打印日志输出
    ~Singleton() {
        std::cout << "this is singleton destruct" << std::endl;
    }
};

//如果直接用Singleton::GetInstance()编译通过，但是链接出现问题
//因为类中static变量（static std::shared_ptr<T> _instance;）需要被初始化，但这里只有声明
//如果static变量不是一个模板类，需要在cpp中初始化，如果是模板类，需要在.h文件初始化
template <typename T>
std::shared_ptr<T> Singleton<T>::_instance = nullptr;//类内声明，类外初始化

#endif // SINGLETON_H
