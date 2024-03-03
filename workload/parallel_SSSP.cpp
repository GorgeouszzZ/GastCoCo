#include "../CBList/CBList.hpp"
#include "../other/atomic.hpp"
#include "Slice_for_CBL_coro.hpp"
#include <iostream>
#include <stdio.h>
#include <chrono>
#include <stack>
#include <vector>
#include "omp.h"

using namespace std;

uint32_t thread_num = 4;
void SSSP(const CoroGraph::CBList& cbl, const VertexID& left, const VertexID& right, vector<int>& node_state, int& clean_flag, int32_t iter)
{
    // std::vector<pair<VertexID, VertexID>> record;
    while(iter)
    {
        --iter;
        //--debug--
        // cout<<iter<<": record->"<<record.size()<<": "<<endl;
        // if(record.size() == 0 && iter != 9999)
        //     break;
        // for(auto& rec:record)
        //     cout<<rec.first<<" -- "<<rec.second<<endl;
        // cout<<endl;
        // record.clear();
        //--debug--
        VertexID now_node = left;
        int nextFlag = 0;
        if(cbl.NodeList[left].Level == 1) nextFlag = CHUNK_LEVEL;
        else if(cbl.NodeList[left].Level == 2) nextFlag = LEAFCHUNK_LEVEL;
        auto nextPtr_tmp = &(cbl.NodeList[left].Neighboor);
        while(now_node != right)
        {         
            if(nextFlag%2 != 0)
            {
                for(int i=0;i<nextPtr_tmp->nextLv1Chunk->count;i++)
                { 
                    int old_V = node_state[nextPtr_tmp->nextLv1Chunk->NeighboorChunk[i].dest];
                    int new_V = node_state[now_node] + nextPtr_tmp->nextLv1Chunk->NeighboorChunk[i].value;
                    if(new_V < old_V)    
                        write_min(&node_state[nextPtr_tmp->nextLv1Chunk->NeighboorChunk[i].dest], new_V);
                    //--debug--
                    // auto dssst = nextPtr_tmp->nextLv1Chunk->NeighboorChunk[i].dest;
                    // if(new_V < old_V)
                    // {
                        // record.emplace_back(now_node, dssst);
                    //     cout << "src: " << now_node << " dst: "<<dssst  \
                    //     <<" value: "<< nextPtr_tmp->nextLv1Chunk->NeighboorChunk[i].value\
                    //     << " ns[src]: " << node_state[now_node] << " oldV: " << old_V<<endl; 
                    // }
                    //--debug--
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
                        write_min(&node_state[nextPtr_tmp->nextLeafChunk->NeighboorChunk[i].dest], new_V);
                    //--debug--
                    // auto dssst = nextPtr_tmp->nextLeafChunk->NeighboorChunk[i].dest;
                    // if(new_V < old_V)
                    // {
                        // record.emplace_back(now_node, dssst);
                    //     cout << "src: " << now_node << " dst: "<<dssst  \
                    //     <<" value: "<< nextPtr_tmp->nextLeafChunk->NeighboorChunk[i].value\
                    //     << " ns[src]: " << node_state[now_node] << " oldV: " << old_V<<endl; 
                    // }
                    //--debug--
                }
                nextFlag = nextPtr_tmp->nextLeafChunk->nextType;
                nextPtr_tmp = &(nextPtr_tmp->nextLeafChunk->nextPtr);
            }
            if(nextFlag<0) now_node++;
        }
        __sync_add_and_fetch(&clean_flag, 1);
        if(clean_flag == thread_num) clean_flag = 0;
        while(clean_flag > 0) {};//同步屏障
        //cout<<"remain:"<<iter<<endl;
    }
}

int main(int argc, char ** argv)
{
    if(argc<5)
    {
        cout<<"[efile] [edge_num] [node_num] [sssp_source] [iter]\n";
        exit(-1);
    }
    string GRAPH_DIR = argv[1];
    string GRAPH_INFO_DIR = argv[2];
    auto [V, E] = LoadEVFromInfoFile(GRAPH_INFO_DIR);
    int source = atoi(argv[3]);
    int iter = atoi(argv[4]);
    thread_num = atoi(argv[5]);
    // iter *= thread_num;
    CoroGraph::CBList G(V, E, GRAPH_DIR);
    auto SFCresult = SliceForCoro(G, thread_num);
    vector<int> node_state(G.NodeNum, 100000);
    node_state[source] = 0;

//==============================================================time_start====================
    auto start = std::chrono::steady_clock::now();//if compute new coroutine spending time?

    std::cout<<thread_num<<std::endl;
    omp_set_num_threads(thread_num);

    int clean_flag = 0;
    #pragma omp parallel for
    for(uint32_t tid=0; tid<thread_num; ++tid)
    {
        SSSP(G, SFCresult[tid], SFCresult[tid+1], node_state, clean_flag, iter);
    }

    auto end = std::chrono::steady_clock::now();
//==============================================================================time_stop===========================
    std::chrono::duration<double, std::micro> elapsed = end - start; // std::micro 表示以微秒为时间单位
    std::cout<< "time: "  << elapsed.count() << "us" << std::endl;
    std::cout<<node_state[6]<<std::endl;
    return EXIT_SUCCESS;
}