#pragma once

#include<iostream>
#include<fstream>
#include<sstream>
#include<string>
#include<vector>
#include<stdio.h>
#include<coroutine>
#include<stack>
#include "../CBList/CBList.hpp"
#include "../other/Coro_graph_v2.hpp"
#include "../other/atomic.hpp"
using namespace std;

#define GET_NUM 64

inline void prefetch(const void* ptr) 
{
    typedef struct { char x[GET_NUM]; } cacheline_t;
    asm volatile("prefetcht0 %0" : : "m"(*(const cacheline_t*)ptr));
  //__builtin_prefetch(*(const cacheline_t*)ptr));
}

void prefetch_Chunk(const void* ptr)
{
    for (int i = 0; i < 4 * 64; i += 64)
      prefetch((const char*)ptr + i);
}


generator<void> SSSP(const CoroGraph::CBList& cbl,const VertexID& left,const VertexID& right, vector<int>& node_state, bool pre_flag, int& clean_flag, const uint32_t& coro_num, int32_t iter)
{
    while(iter != 0)
    {
        --iter;
        VertexID now_node = left;
        int nextFlag = 0;
        if(cbl.NodeList[left].Level == 1) nextFlag = CHUNK_LEVEL;
        else if(cbl.NodeList[left].Level == 2) nextFlag = LEAFCHUNK_LEVEL;
        auto nextPtr_tmp = &(cbl.NodeList[left].Neighboor);
        while(now_node != right)
        {         
            if(nextFlag%2 != 0)
            {
                // prefetch(nextPtr_tmp->nextLv1Chunk);
                co_await suspend_always{};
                for(int i=0;i<nextPtr_tmp->nextLv1Chunk->count;i++)
                { 
                    int old_V = node_state[nextPtr_tmp->nextLv1Chunk->NeighboorChunk[i].dest];
                    int new_V = node_state[now_node] + nextPtr_tmp->nextLv1Chunk->NeighboorChunk[i].value;
                    if(new_V < old_V)    
                        write_min(&node_state[nextPtr_tmp->nextLv1Chunk->NeighboorChunk[i].dest], new_V);
                }
                nextFlag = nextPtr_tmp->nextLv1Chunk->nextType;
                nextPtr_tmp = &(nextPtr_tmp->nextLv1Chunk->nextPtr);                 
            }
            else
            {
                // prefetch_Chunk(nextPtr_tmp->nextLeafChunk);
                co_await suspend_always{}; 
                for(int i=0;i<nextPtr_tmp->nextLeafChunk->count;i++)
                { 
                    int old_V = node_state[nextPtr_tmp->nextLeafChunk->NeighboorChunk[i].dest];
                    int new_V = node_state[now_node] + nextPtr_tmp->nextLeafChunk->NeighboorChunk[i].value;
                    if(new_V < old_V)    
                        write_min(&node_state[nextPtr_tmp->nextLeafChunk->NeighboorChunk[i].dest], new_V);
                }
                nextFlag = nextPtr_tmp->nextLeafChunk->nextType;
                nextPtr_tmp = &(nextPtr_tmp->nextLeafChunk->nextPtr);
            }
            if(nextFlag<0) ++now_node;
        }
        __sync_add_and_fetch(&clean_flag, 1);
        if(clean_flag == coro_num) clean_flag = 0;
        while(clean_flag > 0) co_await suspend_always{};//同步屏障
        //cout<<"remain:"<<iter<<endl;
    }
    //co_await suspend_always{};
}
