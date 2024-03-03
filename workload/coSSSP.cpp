#include "../Bind_cpu.hpp"
#include "../Initalize_container.hpp"
#include "Slice_for_CBL_coro.hpp"
#include <iostream>
#include <stdio.h>
#include <chrono>
#include "./coSSSP.hpp"
using namespace std;

thread_local tcalloc coroutine_allocator;

//best coro:60 prefetch:64 

int main(int argc, char ** argv)
{
    if(argc<6)
    {
        cout<<"[efile] [info_file] [sssp_source] [iter] [coro_num]\n";//graph type
        exit(-1);
    }
    string GRAPH_DIR = argv[1];
    auto [V, E]
    int E = atoi(argv[2]);
    int V = atoi(argv[3]);
    int source = atoi(argv[4]);
    int iter = atoi(argv[5]);
    int coro_num = atoi(argv[6]);
    iter = coro_num * iter;
    BindCpu(2);
    CBList test0(V,E,GRAPH_DIR);
    vector<int> node_state(test0.Node_num);
    cout<<node_state.size()<<endl;
    init_vec(node_state, 100000);
    node_state[source] = 0;
    vector<coroutine_handle<>> sssp_tasks(coro_num);
    int now_node = 0;
    
    int* segment = nullptr;
    slice_for_coro(segment, coro_num, test0);
    int segment_flag = 0;
    int clean = 0;
    for(auto &i : sssp_tasks)
    {
        i=SSSP(test0, segment[segment_flag], segment[segment_flag+1], node_state, true, clean, coro_num, iter).get_handle();
        segment_flag++;
    }
    
    //==============================================================time_start====================
    auto start = std::chrono::steady_clock::now();//if compute new coroutine spending time?

    
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

    auto end = std::chrono::steady_clock::now();
//==============================================================================time_stop===========================
    std::chrono::duration<double, std::micro> elapsed = end - start; // std::micro 表示以微秒为时间单位
    std::cout<< "time: "  << elapsed.count() << "us" << std::endl;
    cout<<node_state[6];
    return EXIT_SUCCESS;
}

