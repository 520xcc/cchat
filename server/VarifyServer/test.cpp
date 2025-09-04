#include <memory>
#include <iostream>
using namespace std;

{
    
} // namespace name

struct A;
struct B;

struct A {
    std::shared_ptr<B> bptr;
    ~A() { std::cout << "A destroyed\n"; }
};

struct B {
    std::weak_ptr<A> aptr;  // 改成 weak_ptr
    ~B() { std::cout << "B destroyed\n"; }
};

int main() {
    auto a = std::make_shared<A>();
    auto b = std::make_shared<B>();

    a->bptr = b;
    b->aptr = a;

    // 正常析构
}
