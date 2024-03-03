#include "../CBList/CBList.hpp"
#include <iostream>
#include <stdio.h>
#include <chrono>
#include <stack>
#include <queue>

using namespace std;


int main(int argc, char ** argv)
{
    if(argc<4)
    {
        std::cout<<"[efile] [infofile] [insert_rate]\n";
        exit(-1);
    }
    string GRAPH_DIR = argv[1];
    string GRAPH_INFO_DIR = argv[2];
    auto [V, E] = LoadEVFromInfoFile(GRAPH_INFO_DIR);
    int insert_rate = atoi(argv[3]);

    std::vector<Gedge> InsertTarget;

    int preload_E = E * (100 - insert_rate) / 100;
    int remain_E = E - preload_E;

    CoroGraph::CBList G(V, preload_E, GRAPH_DIR, E, InsertTarget);
    printf("load graph finish\n");

    //==============================================================time_start====================
    auto start = std::chrono::steady_clock::now();//if compute new coroutine spending time?
    
    for(auto& e:InsertTarget)
        G.InsertEdge(e.start_point, AdjUnit({e.end_point, e.value}));

    auto end = std::chrono::steady_clock::now();
//==============================================================================time_stop===========================
    std::chrono::duration<double, std::micro> elapsed = end - start; // std::micro 表示以微秒为时间单位
    std::cout<< "time: "  << elapsed.count() << "us" << std::endl;

    return EXIT_SUCCESS;
}