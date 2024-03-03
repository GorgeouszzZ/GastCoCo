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
double d = 0.85;

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


generator<void> pagerank(const CoroGraph::CBList& cbl,const VertexID& left,const VertexID& right,vector<double>& node_state_old, vector<double>& node_state_new, bool pre_flag, int32_t iter, int& clean_flag, const uint32_t& coro_num)
{
    while(iter != 0)
    {
        --iter;
        VertexID now_node = left;
        int nextFlag = 0;
        if(cbl.NodeList[left].Level == 1) nextFlag = CHUNK_LEVEL;
        else if(cbl.NodeList[left].Level == 2) nextFlag = LEAFCHUNK_LEVEL;
        auto nextPtr_tmp = &(cbl.NodeList[left].Neighboor);
        int outDegree = cbl.NodeList[left].NeighboorCnt;
        double tmpPR = node_state_old[left] / outDegree;

        // //-----profiling-----
        // double Lv1time = 0.0;
        // double Leaftime = 0.0;
        // double Leaftime2 = 0.0;
        // int Lv1cnt = 0;
        // int Leafcnt = 0;
        // auto startall = std::chrono::steady_clock::now();
        // //-----profiling-----

        while(now_node != right)
        {         
            if(nextFlag%2 != 0)
            {

                // //-----profiling-----
                // ++Lv1cnt;
                // auto startLv1 = std::chrono::steady_clock::now();
                // //-----profiling-----

                // prefetch(nextPtr_tmp->nextLv1Chunk);
                // co_await suspend_always{};
                for(int i=0;i<nextPtr_tmp->nextLv1Chunk->count;i++)
                { 
                    double delta = tmpPR * d;
                    write_add(&node_state_new[nextPtr_tmp->nextLv1Chunk->NeighboorChunk[i].dest], delta);
                    // if(nextPtr_tmp->nextLv1Chunk->NeighboorChunk[i].dest == 0)
                    // {
                    //     printf("pr:%f + (%d)->tmp:%f   ",node_state_new[0],now_node,tmpPR);
                    //     printf("from %d-%d\n",left,right);
                    // }
                }
                nextFlag = nextPtr_tmp->nextLv1Chunk->nextType;
                nextPtr_tmp = &(nextPtr_tmp->nextLv1Chunk->nextPtr);

                // //-----profiling-----
                // auto endLv1 = std::chrono::steady_clock::now();
                // std::chrono::duration<double, std::micro> elapsedLv1 = endLv1 - startLv1;
                // Lv1time += elapsedLv1.count();
                // //-----profiling-----

            }
            else
            {

                // //-----profiling-----
                // auto startLeafsuspend = std::chrono::steady_clock::now();
                // //-----profiling-----

                // prefetch_Chunk(nextPtr_tmp->nextLeafChunk);
                co_await suspend_always{};

                // //-----profiling-----
                // ++Leafcnt;
                // auto startLeaf = std::chrono::steady_clock::now();
                // //-----profiling-----

                for(int i=0;i<nextPtr_tmp->nextLeafChunk->count;i++)
                { 
                    double delta = tmpPR * d;
                    write_add(&node_state_new[nextPtr_tmp->nextLeafChunk->NeighboorChunk[i].dest], delta);
                    // if(nextPtr_tmp->nextLeafChunk->NeighboorChunk[i].dest == 0)
                    // {
                    //     printf("pr:%f + (%d)->tmp:%f",node_state_new[0],now_node,tmpPR);
                    //     printf("from %d-%d\n",left,right);
                    // }
                }
                nextFlag = nextPtr_tmp->nextLeafChunk->nextType;
                nextPtr_tmp = &(nextPtr_tmp->nextLeafChunk->nextPtr);

                // //-----profiling-----
                // auto endLeaf = std::chrono::steady_clock::now();
                // std::chrono::duration<double, std::micro> elapsedLeaf = endLeaf - startLeaf;
                // std::chrono::duration<double, std::micro> elapsedLeaf2 = endLeaf - startLeafsuspend;
                // Leaftime += elapsedLeaf.count();
                // Leaftime2 += elapsedLeaf2.count();
                // //-----profiling-----

            }
            if(nextFlag<0)
            {
                ++now_node;
                outDegree = cbl.NodeList[now_node].NeighboorCnt;
                tmpPR = node_state_old[now_node] / outDegree;
            }
        }

        // //-----profiling-----
        // auto endall = std::chrono::steady_clock::now();
        // std::chrono::duration<double, std::micro> elapsedall = endall - startall;
        // std::cout<<"lv1cnt: "<<Lv1cnt<<" leafcnt: "<<Leafcnt<<" lv1time: "<<Lv1time<<" leaftime: "<<Leaftime<<" leaftime2: "<<Leaftime2<<std::endl;
        // std::cout<<"alltime: "<<elapsedall.count()<<std::endl;
        // //-----profiling-----

        __sync_add_and_fetch(&clean_flag, 1);
        // cout<<" remain:"<<omp_get_thread_num()<<" "<<clean_flag<<" "<<iter<<endl;
        if(clean_flag == coro_num)
        {
            for(int i=0;i<node_state_old.size();i++)
            {
                double new_old = abs(node_state_new[i] - node_state_old[i]);
                // if(abs(page.newPR - page.oldPR) > threshold) //可以在这里设置阈值判断收敛
                //     shouldStop = 0;
                node_state_old[i] = node_state_new[i];
                node_state_new[i]= 1-d;
                // if(i==6) std::cout<<node_state_old[i]<<" \n";
            }
            clean_flag = 0;
        }
        while(clean_flag > 0 && iter != 0) co_await suspend_always{};//同步屏障
        // cout<<" remain:"<<iter<<endl;
    }
    //co_await suspend_always{};
}
