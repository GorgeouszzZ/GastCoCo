#include "Slice_for_CBL_coro.hpp"
#include <iostream>
#include <stdio.h>
#include <chrono>
#include "./coSSSP.hpp"
#include "omp.h"
using namespace std;

thread_local tcalloc coroutine_allocator;

//best coro:60 prefetch:64 

int main(int argc, char ** argv)
{
    if(argc<6)
    {
        cout<<"[efile] [edge_num] [node_num] [sssp_source] [iter] [coro_num]\n";//graph type
        exit(-1);
    }
    string GRAPH_DIR = argv[1];
    string GRAPH_INFO_DIR = argv[2];
    auto [V, E] = LoadEVFromInfoFile(GRAPH_INFO_DIR);
    int source = atoi(argv[3]);
    int iter = atoi(argv[4]);
    int coro_num = atoi(argv[5]);
    uint32_t thread_num = atoi(argv[6]);
    CoroGraph::CBList G(V,E,GRAPH_DIR);
    printf("load graph finish\n");
    uint32_t all_coro_num = coro_num * thread_num;
    // iter *= all_coro_num; 
    vector<int> node_state(G.NodeNum, 100000);
    node_state[source] = 0;
    uint32_t now_node = 0;
    
    auto SFCresult = SliceForCoro(G, all_coro_num);


    //==============================================================time_start====================
    auto start = std::chrono::steady_clock::now();

    std::cout<<thread_num<<std::endl;
    omp_set_num_threads(thread_num);

    int clean_flag = 0;
    #pragma omp parallel for
    for(uint32_t tid=0;tid<thread_num;++tid)
    {
        vector<coroutine_handle<>> SSSPTasks(coro_num);
        for(uint32_t i = 0; i<coro_num; ++i)
        {
            SSSPTasks[i]=SSSP(G, SFCresult[tid * coro_num + i], SFCresult[tid * coro_num + i+1 ], node_state, true, clean_flag, all_coro_num, iter).get_handle();
        }
        // std::cout << tid << omp_get_thread_num() << " process: " << SFCresult[tid * coro_num] << "~" << SFCresult[tid * coro_num + coro_num] << std::endl;
        int finished = 0;
        int flag = 0;
        while(finished < SSSPTasks.size())
        {
            for(auto &t : SSSPTasks)
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

    auto end = std::chrono::steady_clock::now();
//==============================================================================time_stop===========================
    std::chrono::duration<double, std::micro> elapsed = end - start; // std::micro 表示以微秒为时间单位
    std::cout<< "time: "  << elapsed.count() << "us" << std::endl;
    cout<<node_state[6]<<endl;
    return EXIT_SUCCESS;
}

