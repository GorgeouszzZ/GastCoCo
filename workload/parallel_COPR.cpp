#include "Slice_for_CBL_coro.hpp"
#include <iostream>
#include <stdio.h>
#include <chrono>
#include "copagerank.hpp"
#include <omp.h>
using namespace std;

thread_local tcalloc coroutine_allocator;

void COPR(vector<coroutine_handle<>>& PrTasks)
{
    int finished = 0;
    int flag = 0;
    while(finished < PrTasks.size())
    {
        for(auto &t : PrTasks)
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



int main(int argc, char ** argv)
{
    string GRAPH_DIR = argv[1];
    string GRAPH_INFO_DIR = argv[2];
    auto [V, E] = LoadEVFromInfoFile(GRAPH_INFO_DIR);
    int iter = atoi(argv[3]);
    int coro_num = atoi(argv[4]);
    int thread_num = atoi(argv[5]);
    CoroGraph::CBList G(V,E,GRAPH_DIR);
    printf("load graph finish\n");
    uint32_t all_coro_num = coro_num * thread_num;
    vector<vector<coroutine_handle<>>> AllPrTasks(thread_num);
    // iter *= all_coro_num; 
    vector<double> node_state_old(G.NodeNum, (double)1);
    vector<double> node_state_new(G.NodeNum, 1-d);
    uint32_t now_node = 0;
    
    auto SFCresult = SliceForCoro(G, all_coro_num);
    int clean_flag = 0;
    for(uint32_t tid=0;tid<thread_num;++tid)
    {

        for(uint32_t i = 0; i<coro_num; ++i)
        {
            AllPrTasks[tid].emplace_back(pagerank(G, SFCresult[tid * coro_num + i], SFCresult[tid * coro_num + i+1 ], node_state_old, node_state_new, true, iter, clean_flag, all_coro_num).get_handle());
        }
        // std::cout << tid << omp_get_thread_num() << " process: " << SFCresult[tid * coro_num] << "~" << SFCresult[tid * coro_num + coro_num] << std::endl;
    } 


    //==============================================================time_start====================
    auto start = std::chrono::steady_clock::now();

    std::cout<<thread_num<<std::endl;
    omp_set_num_threads(thread_num);

    #pragma omp parallel for
    for(uint32_t tid=0;tid<thread_num;++tid)
        COPR(AllPrTasks[tid]);
    

    auto end = std::chrono::steady_clock::now();
//==============================================================================time_stop===========================
    std::chrono::duration<double, std::micro> elapsed = end - start; // std::micro 表示以微秒为时间单位
    std::cout<< "time: "  << elapsed.count() << "us" << std::endl;
    cout<<node_state_old[6]<<endl;
    return EXIT_SUCCESS;
}

