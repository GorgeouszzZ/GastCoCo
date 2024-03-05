#include "../other/Coro_graph_v2.hpp"
#include <iostream>
#include <functional>
#include <coroutine>
using namespace std;

thread_local tcalloc coroutine_allocator;

generator<void> get_num(int x)
{
    int n = 3;
    while(n--)
    {
        cout << "this->coro" << x << endl;
        ++x;
        co_await suspend_always{};
    }
}

class Gen
{
public:
    void func(function<generator<void>(int)> coro)
    {
        vector<coroutine_handle<>> mm(3);
        for(int i = 0; i < 3; ++i)
            mm[i] = coro(i).get_handle();
        SimpleScheduler(mm);
    }
};

int main()
{
    Gen a;
    a.func(get_num);
    cout << sizeof(unsigned long) << endl;
}