#include "../other/Bind_cpu.hpp"
#include "Slice_for_CBL_coro.hpp"
#include <iostream>
#include <stdio.h>
#include <chrono>
#include "copagerank.hpp"
using namespace std;

thread_local tcalloc coroutine_allocator;

//best coro:60 prefetch:64 

int main(int argc, char ** argv)
{
    if(argc<6)
    {
        cout<<"[efile] [edge_num] [node_num] [iter] [coro_num]\n";//graph type
        exit(-1);
    }
    string GRAPH_DIR = argv[1];
    int E = atoi(argv[2]);
    int V = atoi(argv[3]);
    int iter = atoi(argv[4]);
    int coro_num = atoi(argv[5]);
    iter = coro_num * iter;
    BindCpu(2);
    CoroGraph::CBList test0(V,E,GRAPH_DIR);
    printf("load graph finish\n");
    vector<double> node_state_old(test0.NodeNum, (double)1);
    vector<double> node_state_new(test0.NodeNum, 1-d);
    vector<coroutine_handle<>> pr_tasks(coro_num);
    int now_node = 0;
    
    // int* segment = nullptr;
    // slice_for_coro(segment, coro_num, test0);
    // std::vector<unsigned int> segment(3,0);
    // // std::vector<uint32_t> segment(3,0);
    // segment[0] = 0;
    // segment[1] = 672154;
    // segment[2] = 1139905;
    auto segment = SliceForCoro(test0, coro_num);
    // auto segment = SFC();
    int segment_flag = 0;
    int clean_flag = 0;
    for(auto &i : pr_tasks)
    {
        printf("%d, %d\n", segment[segment_flag], segment[segment_flag+1]);
        i=pagerank(test0, segment[segment_flag], segment[segment_flag+1], node_state_old, node_state_new, true, iter, clean_flag, coro_num).get_handle();
        segment_flag++;
    }
    
    //==============================================================time_start====================
    auto start = std::chrono::steady_clock::now();//if compute new coroutine spending time?

    
    int finished = 0;
    while(finished < pr_tasks.size())
    {
        for(auto &t : pr_tasks)
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
    cout<<node_state_old[6]<<endl;
    return EXIT_SUCCESS;
}

