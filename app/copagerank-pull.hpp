//#include "../Bind_cpu.hpp"
#include "../other/Slice_for_CBL_coro.hpp"
#include <iostream>
#include <stdio.h>
#include <chrono>
#include "copagerank.hpp"
#include <omp.h>
using namespace std;

thread_local tcalloc coroutine_allocator;

void pagerank(GastCoCo::CBList& graph, int thread_num, int coro_num, int iter)
{
    uint32_t all_coro_num = coro_num * thread_num;
    // iter *= all_coro_num; 
    vector<double> vertex_state_old(graph.VertexNum, (double)1);
    vector<double> vertex_state_new(graph.VertexNum, 1-d);
    uint32_t now_vertex = 0;
    
    auto partition_graph_result = SliceForCoro(graph, all_coro_num);

    //==============================================================time_start====================
    auto start = std::chrono::steady_clock::now();

    #ifdef DEBUG
        std::cout<<"thread: "<<thread_num<<std::endl;
    #endif

    omp_set_num_threads(thread_num);

    for(GastCoCo::IndexType it = 0; it < iter; ++it)
    {
        #pragma omp parallel for
        for(uint32_t tid = 0; tid < thread_num; ++tid)
        {
            vector<coroutine_handle<>> PrTasks(coro_num);
            for(uint32_t i = 0; i<coro_num; ++i)
            {
                // PrTasks[i]=pagerank_oi_hybrid1(G, partition_graph_result[tid * coro_num + i], partition_graph_result[tid * coro_num + i+1 ], vertex_state_old, vertex_state_new, true).get_handle();
                PrTasks[i]=pagerank_one_iter(graph, partition_graph_result[tid * coro_num + i], partition_graph_result[tid * coro_num + i+1 ], vertex_state_old, vertex_state_new, true).get_handle();
            }
            // std::cout << tid << omp_get_thread_num() << " process: " << partition_graph_result[tid * coro_num] << "~" << partition_graph_result[tid * coro_num + coro_num] << std::endl;
            int finished = 0;
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

        swap(vertex_state_new, vertex_state_old);
        #pragma omp parallel for
        for(GastCoCo::VertexID vid=0; vid<graph.VertexNum; ++vid)
            vertex_state_new[vid] = 1-d;
    }

    auto end = std::chrono::steady_clock::now();
//==============================================================================time_stop===========================
    std::chrono::duration<double, std::micro> elapsed = end - start; // std::micro 表示以微秒为时间单位
    std::cout<< "time: "  << elapsed.count() << "us" << std::endl;
    cout<<vertex_state_old[6]<<endl;
}

