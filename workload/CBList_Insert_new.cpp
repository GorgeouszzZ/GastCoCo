#include "../other/Bind_cpu.hpp"
#include "../CBList/CBList.hpp"
#include <iostream>
#include <stdio.h>
#include <chrono>
#include <stack>
#include <queue>
#include "../other/atomic.hpp"

using namespace std;

void InsertGraphNew(CoroGraph::CBList cblist, Gedge* InsertTarget,const int& InsertNum)
{
    int process_pos = 0;
    while(process_pos!=InsertNum)
    {
        auto InsertINFO = InsertTarget[process_pos];
        AdjUnit InsertINFO_adj = { InsertINFO.end_point, InsertINFO.value };
        cblist.Insert(InsertINFO.start_point, InsertINFO_adj);
        ++process_pos;
    }
}

int main(int argc, char ** argv)
{
    if(argc<5)
    {
        cout<<"[efile] [edge_num(start_line)] [node_num] [end_line]\n";
        exit(-1);
    }
    string GRAPH_DIR = argv[1];
    int E = atoi(argv[2]);
    int V = atoi(argv[3]);
    int EndLine = atoi(argv[4]);
    BindCpu(2);
    CBList test1(V, E, GRAPH_DIR);

    Gedge* InsertList = nullptr; 
    InsertList = test1.GetInsertList(GRAPH_DIR, E, EndLine);
    int InsertNum = EndLine-E;
    //==============================================================time_start====================
    auto start = std::chrono::steady_clock::now();

    InsertGraphNew(test1, InsertList, InsertNum);

    auto end = std::chrono::steady_clock::now();
//==============================================================================time_stop===========================
    std::chrono::duration<double, std::micro> elapsed = end - start; // std::micro 表示以微秒为时间单位
    std::cout<< "time: " << elapsed.count() << "us" << std::endl;

    return EXIT_SUCCESS;
}