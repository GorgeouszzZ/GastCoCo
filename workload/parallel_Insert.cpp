#include "../other/Bind_cpu.hpp"
#include "../CBList/CBList.hpp"
#include <iostream>
#include <stdio.h>
#include <chrono>
#include <stack>
#include <queue>
#include "../other/atomic.hpp"
#include "Slice_for_CBL_coro.hpp"

using namespace std;
vector<Gedge> InsertTarget;


void InsertGraphNew(CoroGraph::CBList& cblist, const EdgeID& left, const EdgeID& right)
{
    int process_pos = left;
    while(process_pos!=right)
    {
        auto InsertINFO = InsertTarget[process_pos];
        cblist.writelock[InsertINFO.start_point].lock();
        AdjUnit InsertINFO_adj = { InsertINFO.end_point, InsertINFO.value };
        cblist.InsertEdge(InsertINFO.start_point, InsertINFO_adj);
        cblist.writelock[InsertINFO.start_point].unlock();
        ++process_pos;
    }
}

int main(int argc, char ** argv)
{
    if(argc<4)
    {
        cout<<"[efile] [edge_num] [node_num] [insert_rate]\n";
        exit(-1);
    }
    string GRAPH_DIR = argv[1];
    string GRAPH_INFO_DIR = argv[2];
    auto [V, E] = LoadEVFromInfoFile(GRAPH_INFO_DIR);
    int insert_rate = atoi(argv[3]);
    uint32_t thread_num = 4;

    int preload_E = E * (100 - insert_rate) / 100;
    int remain_E = E - preload_E;

    CoroGraph::CBList G(V, preload_E, GRAPH_DIR, E, InsertTarget);
    printf("load graph finish\n");
    auto TaskResult = SliceTaskForCoro(remain_E, thread_num);

    //==============================================================time_start====================
    auto start = std::chrono::steady_clock::now();

    std::cout<<thread_num<<std::endl;
    omp_set_num_threads(thread_num);
    #pragma omp parallel for
    for(int i=0;i<thread_num;i++)
    {
        InsertGraphNew(G, TaskResult[i], TaskResult[i+1]);
    }

    auto end = std::chrono::steady_clock::now();
//==============================================================================time_stop===========================
    std::chrono::duration<double, std::micro> elapsed = end - start; // std::micro 表示以微秒为时间单位
    std::cout<< "time: " << elapsed.count() << "us" << std::endl;

    return EXIT_SUCCESS;
}