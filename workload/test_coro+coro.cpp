#include "../Bind_cpu.hpp"
#include "CBList.hpp"
#include "../Initalize_container.hpp"
#include <iostream>
#include <stdio.h>
#include <chrono>
#include <stack>
#include <queue>
#include "../Coro_graph_v2.hpp"

using namespace std;

thread_local tcalloc coroutine_allocator;

generator<void> A(const int& coro_No)
{
    cout<<coro_No<<"this is a1\n";
    co_await suspend_always{};
    cout<<coro_No<<"this is a2\n";
    co_await suspend_always{};
    cout<<coro_No<<"this is a3\n";
}

generator<void> B(const int coro_No)
{
    cout<<coro_No<<"this is b1\n";
    auto t = A(coro_No).get_handle();
    while(t)
    {   
        if(t.done())
        {
            t.destroy();
            t = nullptr;
        }
        else
        {
            t.resume();
            co_await suspend_always{};
        }
    }
    cout<<coro_No<<"this is b2\n";
    co_await suspend_always{};
    cout<<coro_No<<"this is b3\n";
}

int main()
{
    vector<coroutine_handle<>> sssp_tasks(2);
    int k=0;
    for(auto &i : sssp_tasks)
    {
        i=B(k++).get_handle();
    }
    int finished = 0;
    while(finished < sssp_tasks.size())
    {
        for(auto &t : sssp_tasks)
        {
            if(t)
            {
                if(t.done())
                {
                    finished++;
                    t.destroy();
                    t = nullptr;
                }
                else
                {
                    t.resume();
                }
            }
        }
    }
}