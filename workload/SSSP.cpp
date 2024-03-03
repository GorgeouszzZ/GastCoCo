#include "../Bind_cpu.hpp"
#include "CBList.hpp"
#include "../Initalize_container.hpp"
#include <iostream>
#include <stdio.h>
#include <chrono>
#include <stack>

using namespace std;

void SSSP(CBList cbl, vector<int>& node_state, int iter)
{
    while(iter)
    {
        int now_node = 0;
        int nextFlag = 0;
        if(cbl.NodeList[0].Level == 1) nextFlag = CHUNK_LEVEL;
        else if(cbl.NodeList[0].Level == 2) nextFlag = LEAFCHUNK_LEVEL;
        auto nextPtr_tmp = &(cbl.NodeList[0].Neighboor);
        while(now_node != cbl.Node_num)
        {         
            if(nextFlag%2 != 0)
            {
                for(int i=0;i<nextPtr_tmp->nextLv1Chunk->count;i++)
                { 
                    int old_V = node_state[nextPtr_tmp->nextLv1Chunk->NeighboorChunk[i].dest];
                    int new_V = node_state[now_node] + nextPtr_tmp->nextLv1Chunk->NeighboorChunk[i].value;
                    if(new_V < old_V)    
                        node_state[nextPtr_tmp->nextLv1Chunk->NeighboorChunk[i].dest] = new_V;
                }
                nextFlag = nextPtr_tmp->nextLv1Chunk->nextType;
                nextPtr_tmp = &(nextPtr_tmp->nextLv1Chunk->nextPtr);                 
            }
            else
            {
                for(int i=0;i<nextPtr_tmp->nextLeafChunk->count;i++)
                { 
                    int old_V = node_state[nextPtr_tmp->nextLeafChunk->NeighboorChunk[i].dest];
                    int new_V = node_state[now_node] + nextPtr_tmp->nextLeafChunk->NeighboorChunk[i].value;
                    if(new_V < old_V)    
                        node_state[nextPtr_tmp->nextLeafChunk->NeighboorChunk[i].dest] = new_V;
                }
                nextFlag = nextPtr_tmp->nextLeafChunk->nextType;
                nextPtr_tmp = &(nextPtr_tmp->nextLeafChunk->nextPtr);
            }
            if(nextFlag<0) now_node++;
        }
        iter--;
        //cout<<"remain:"<<iter<<endl;
    }
}

int main(int argc, char ** argv)
{
    if(argc<6)
    {
        cout<<"[efile] [edge_num] [node_num] [sssp_source] [iter]\n";
        exit(-1);
    }
    string GRAPH_DIR = argv[1];
    int E = atoi(argv[2]);
    int V = atoi(argv[3]);
    int source = atoi(argv[4]);
    int iter = atoi(argv[5]);
    BindCpu(2);
    CBList test1(V, E, GRAPH_DIR);

    vector<int> node_state(test1.Node_num);
    init_vec(node_state, 100000);
    node_state[source] = 0;

//==============================================================time_start====================
    auto start = std::chrono::steady_clock::now();//if compute new coroutine spending time?

    SSSP(test1, node_state, iter);

    auto end = std::chrono::steady_clock::now();
//==============================================================================time_stop===========================
    std::chrono::duration<double, std::micro> elapsed = end - start; // std::micro 表示以微秒为时间单位
    std::cout<< "time: "  << elapsed.count() << "us" << std::endl;
    std::cout<<node_state[6]<<std::endl;
    return EXIT_SUCCESS;
}