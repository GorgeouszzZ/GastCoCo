#include "../other/Bind_cpu.hpp"
#include "Slice_for_CBL_coro.hpp"
#include <iostream>
#include <stdio.h>
#include <chrono>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <coroutine>
#include <stack>
#include "../CBList/CBList.hpp"
#include "../other/Coro_graph_v2.hpp"
#include "../other/atomic.hpp"
using namespace std;

/*
    用于阿里服务器上证明coro+prefetch的有效性。


*/
thread_local tcalloc coroutine_allocator;

//best coro:60 prefetch:64 

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


generator<void> pagerank(const CoroGraph::CBList& cbl,const VertexID& left,const VertexID& right,vector<double>& node_state_old, vector<double>& node_state_new, bool pre_flag, int32_t& iter, int& clean_flag, const uint32_t& coro_num)
{
    while(iter != 0)
    {
        __sync_fetch_and_add(&iter, -1);
        VertexID now_node = left;
        int nextFlag = 0;
        if(cbl.NodeList[left].Level == 1) nextFlag = CHUNK_LEVEL;
        else if(cbl.NodeList[left].Level == 2) nextFlag = LEAFCHUNK_LEVEL;
        auto nextPtr_tmp = &(cbl.NodeList[left].Neighboor);
        int outDegree = cbl.NodeList[left].NeighboorCnt;
        double tmpPR = node_state_old[left] / outDegree;
        while(now_node != right)
        {         
            if(nextFlag%2 != 0)
            {
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
            }
            else
            {
                prefetch_Chunk(nextPtr_tmp->nextLeafChunk);
                co_await suspend_always{};
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
            }
            if(nextFlag<0)
            {
                ++now_node;
                outDegree = cbl.NodeList[now_node].NeighboorCnt;
                tmpPR = node_state_old[now_node] / outDegree;
            }
        }
        __sync_fetch_and_add(&clean_flag, 1);
        if(clean_flag == coro_num)
        {
            clean_flag = 0;
            for(int i=0;i<node_state_old.size();i++)
            {
                double new_old = abs(node_state_new[i] - node_state_old[i]);
                // if(abs(page.newPR - page.oldPR) > threshold) //可以在这里设置阈值判断收敛
                //     shouldStop = 0;
                node_state_old[i] = node_state_new[i];
                node_state_new[i]= 1-d;
                // if(i==6) std::cout<<node_state_old[i]<<" \n";
            }
        }
        while(clean_flag > 0) co_await suspend_always{};//同步屏障
        // cout<<"remain:"<<iter<<endl;
    }
    //co_await suspend_always{};
}

generator<> task_coro(const CoroGraph::CBList& cbl, int coro_id, int task_V)
{
    //code1
    auto s1 = std::chrono::steady_clock::now();
    
    int mult = 1;
    double mm = 1;
    for(int i=0;i<cbl.NodeList[task_V].Neighboor.nextLeafChunk->count;++i)
    {
        auto k = cbl.NodeList[task_V].Neighboor.nextLeafChunk->NeighboorChunk[i];
        mult+=k.dest;
        mm *= k.dest/k.value; 
    }


    auto e1 = std::chrono::steady_clock::now();
    std::chrono::duration<double, std::micro> el1 = e1 - s1;

    prefetch_Chunk(cbl.NodeList[task_V].Neighboor.nextLeafChunk->nextPtr.nextLeafChunk);
    co_await suspend_always{};

    auto s2 = std::chrono::steady_clock::now();

    mult = 1;
    mm = 1;
    auto p2 = cbl.NodeList[task_V].Neighboor.nextLeafChunk->nextPtr.nextLeafChunk;
    for(int i=0;i<p2->count;++i)
    {
        auto k = p2->NeighboorChunk[i];
        mult+=k.dest;
        mm *= k.dest/k.value; 
    }

    auto e2 = std::chrono::steady_clock::now();
    std::chrono::duration<double, std::micro> el2 = e2 - s2;

    std::cout<<coro_id<<" "<<el1.count()<<"\n"<<el2.count()<<std::endl;
}

void task_nocoro(const CoroGraph::CBList& cbl)
{
    auto s1 = std::chrono::steady_clock::now();
    
    int mult = 1;
    double mm = 1;
    for(int i=0;i<cbl.NodeList[61].Neighboor.nextLeafChunk->count;++i)
    {
        auto k = cbl.NodeList[61].Neighboor.nextLeafChunk->NeighboorChunk[i];
        mult+=k.dest;
        mm *= k.dest/k.value; 
    }


    auto e1 = std::chrono::steady_clock::now();
    std::chrono::duration<double, std::micro> el1 = e1 - s1;

    auto s2 = std::chrono::steady_clock::now();

    mult = 1;
    mm = 1;
    auto p1 = cbl.NodeList[61].Neighboor.nextLeafChunk->nextPtr.nextLeafChunk;
    for(int i=0;i<p1->count;++i)
    {
        auto k = p1->NeighboorChunk[i];
        mult+=k.dest;
        mm *= k.dest/k.value; 
    }


    auto e2 = std::chrono::steady_clock::now();
    std::chrono::duration<double, std::micro> el2 = e2 - s2;
    auto s3 = std::chrono::steady_clock::now();

        mult = 1;
    mm = 1;
    for(int i=0;i<cbl.NodeList[500004].Neighboor.nextLeafChunk->count;++i)
    {
        auto k = cbl.NodeList[500004].Neighboor.nextLeafChunk->NeighboorChunk[i];
        mult+=k.dest;
        mm *= k.dest/k.value; 
    }

    auto e3 = std::chrono::steady_clock::now();
    std::chrono::duration<double, std::micro> el3 = e3 - s3;

    auto s4 = std::chrono::steady_clock::now();


        mult = 1;
    mm = 1;
    auto p2 = cbl.NodeList[500004].Neighboor.nextLeafChunk->nextPtr.nextLeafChunk;
    for(int i=0;i<p2->count;++i)
    {
        auto k = p2->NeighboorChunk[i];
        mult+=k.dest;
        mm *= k.dest/k.value; 
    }
    auto e4 = std::chrono::steady_clock::now();
    std::chrono::duration<double, std::micro> el4 = e4 - s4;

    std::cout<<el1.count()<<"\n"<<el2.count()<<"\n"<<el3.count()<<"\n"<<el4.count()<<std::endl;
}

int main(int argc, char ** argv)
{
    string GRAPH_DIR = argv[1];
    string GRAPH_INFO_DIR = argv[2];
    int options = atoi(argv[3]);
    auto [V, E] = LoadEVFromInfoFIle(GRAPH_INFO_DIR);
    BindCpu(2);
    CoroGraph::CBList test0(V,E,GRAPH_DIR);
    printf("load graph finish\n");

    if(options == 1){
    auto start = std::chrono::steady_clock::now();
    task_nocoro(test0);

    return 1;
    }

    // for(int i=10000;i<10020;++i)
    //     std::cout<<i<<"-"<<test0.NodeList[i].ChunkCnt<<"-"<<test0.NodeList[i].NeighboorCnt<<"  ";
    // for(int i=5000;i<20+5000;++i)
    //     std::cout<<i<<"-"<<test0.NodeList[i].ChunkCnt<<"-"<<test0.NodeList[i].NeighboorCnt<<"  ";
    // cout<<&test0.NodeList[0]<<" "<<&test0.NodeList[1]<<" "<<&test0.NodeList[2]<<endl;
    // for(int i=0;i<3;++i)
    // {
    //     cout<<"======this_is_"<<i<<"======"<<endl;
    //     cout<<&test0.NodeList[i]<<endl;
    //     cout<<"level :\t"<<&test0.NodeList[i].Level<<endl;
    //     cout<<"neighcnt:\t"<<&test0.NodeList[i].NeighboorCnt<<endl;
    //     cout<<"chunkcnt:\t"<<&test0.NodeList[i].ChunkCnt<<endl;
    //     cout<<"bpt:\t"<<&test0.NodeList[i].BPT<<endl;
    //     cout<<"neigh:\t"<<&test0.NodeList[i].Neighboor<<endl;
    //     cout<<"lv1:\t"<<test0.NodeList[i].Neighboor.nextLv1Chunk<<endl;
    //     cout<<"lv1cnt:\t"<<&test0.NodeList[i].Neighboor.nextLv1Chunk->count<<endl;
    //     cout<<"lv1neicnt:\t"<<&test0.NodeList[i].Neighboor.nextLv1Chunk->NeighboorChunk<<endl;
    //     cout<<"lv1nxptr:\t"<<&test0.NodeList[i].Neighboor.nextLv1Chunk->nextPtr<<endl;
    //     cout<<"lv1nxtype:\t"<<&test0.NodeList[i].Neighboor.nextLv1Chunk->nextType<<endl;
    // }
    

    vector<coroutine_handle<>> pr_tasks(2);

    // for(int i=0;i<2;++i)
    // {
        pr_tasks[0]=task_coro(test0, 0, 61).get_handle();
        pr_tasks[1]=task_coro(test0, 1, 500004).get_handle();
    //}
    
//     //==============================================================time_start====================
//     auto start = std::chrono::steady_clock::now();//if compute new coroutine spending time?

    
    int finished = 0;
    while(finished < pr_tasks.size())
    {
        for(auto &t : pr_tasks)
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

//     auto end = std::chrono::steady_clock::now();
// //==============================================================================time_stop===========================
//     std::chrono::duration<double, std::micro> elapsed = end - start; // std::micro 表示以微秒为时间单位
//     std::cout<< "time: "  << elapsed.count() << "us" << std::endl;
//     cout<<node_state_old[6]<<endl;
//     return EXIT_SUCCESS;
}

