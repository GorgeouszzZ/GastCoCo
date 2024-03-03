#include "../CBList/CBList.hpp"
#include <iostream>
#include <stdio.h>
#include <chrono>
#include <stack>
#include <queue>
#include <vector>
#include "../other/Coro_graph_v2.hpp"
#include "Slice_for_CBL_coro.hpp"

using namespace std;

thread_local tcalloc coroutine_allocator;

#define GET_NUM 64

vector<Gedge> InsertTarget;

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

generator<void> CoInsertGraph(CoroGraph::CBList& cblist, const EdgeID left, const EdgeID right)
{
    int process_pos = left;
    while(process_pos != right)
    {
        int this_process = process_pos;
        process_pos++;
        auto InsertINFO = InsertTarget[this_process];
        while(!cblist.writelock[InsertINFO.start_point].try_lock())
        {
            co_await suspend_always{};
        }
        AdjUnit InsertINFO_adj = { InsertINFO.end_point, InsertINFO.value };
        int flag = 0;
        if(cblist.NodeList[InsertINFO.start_point].NeighboorCnt < LV1CHUNK_NCNT)
        {
            cblist.NodeList[InsertINFO.start_point].Neighboor.nextLv1Chunk->NeighboorChunk[cblist.NodeList[InsertINFO.start_point].Neighboor.nextLv1Chunk->count] = InsertINFO_adj;
            cblist.NodeList[InsertINFO.start_point].Neighboor.nextLv1Chunk->count++;
            cblist.NodeList[InsertINFO.start_point].NeighboorCnt++;
            flag = 1;
        }
        else if(cblist.NodeList[InsertINFO.start_point].NeighboorCnt == LV1CHUNK_NCNT)
        {
            cblist.NodeList[InsertINFO.start_point].Level++;
            for(int i=0;i<cblist.NodeList[InsertINFO.start_point].NeighboorCnt;i++)
            {
                cblist.NodeList[InsertINFO.start_point].BPT.Insert(cblist.NodeList[InsertINFO.start_point].Neighboor.nextLv1Chunk->NeighboorChunk[i]);
            }
            cblist.NodeList[InsertINFO.start_point].BPT.Insert({InsertINFO.end_point, InsertINFO.value});
            cblist.NodeList[InsertINFO.start_point].NeighboorCnt++;
            auto LastLeaf = cblist.NodeList[InsertINFO.start_point].BPT.GetLeafEnd();
            LastLeaf->nextType = cblist.NodeList[InsertINFO.start_point].Neighboor.nextLv1Chunk->nextType;
            LastLeaf->nextPtr = cblist.NodeList[InsertINFO.start_point].Neighboor.nextLv1Chunk->nextPtr;
            delete cblist.NodeList[InsertINFO.start_point].Neighboor.nextLv1Chunk;
            cblist.NodeList[InsertINFO.start_point].Neighboor.nextLeafChunk = cblist.NodeList[InsertINFO.start_point].BPT.GetLeafStart();
            cblist.NodeList[InsertINFO.start_point].ChunkCnt = cblist.NodeList[InsertINFO.start_point].BPT.LeafCnt;
            flag = 2;
        }
        else
        {
            cblist.NodeList[InsertINFO.start_point].NeighboorCnt++;
            int kk_forfind = 0;
            CoroGraph::Node* p = cblist.NodeList[InsertINFO.start_point].BPT.Root;
            CoroGraph::InterNode* q;
            while (p != nullptr)
            {
                prefetch_Chunk(p);
                co_await suspend_always{};
                if (p->IsLeaf)
                    break;
                for (kk_forfind = 0; kk_forfind < p->count; kk_forfind++)
                {
                    if (InsertINFO.end_point < ((CoroGraph::InterNode*)p)->KeyList[kk_forfind])
                        break;
                }
                q = (CoroGraph::InterNode*)p;
                p = q->Child[kk_forfind];
            }
            CoroGraph::LeafChunk* InsertPos = (CoroGraph::LeafChunk*)p;
            cblist.NodeList[InsertINFO.start_point].BPT.InsertKnowPos(InsertINFO_adj, InsertPos);
            cblist.NodeList[InsertINFO.start_point].ChunkCnt = cblist.NodeList[InsertINFO.start_point].BPT.LeafCnt;
            flag = 1;
        }
        if(flag == 0){ printf("error"); exit(-1); }
        int src = InsertINFO.start_point;
        if(flag == 2 && src != 0)
        {
            int level = cblist.NodeList[src-1].Level;
            if(level == 1)
            {
                cblist.NodeList[src-1].Neighboor.nextLv1Chunk->nextPtr = cblist.NodeList[src].Neighboor;
                cblist.NodeList[src-1].Neighboor.nextLv1Chunk->nextType = LEAFCHUNK_LEVEL;
            }
            else if(level == 2)
            {
                cblist.NodeList[src-1].BPT.GetLeafEnd()->nextPtr = cblist.NodeList[src].Neighboor;
                cblist.NodeList[src-1].BPT.GetLeafEnd()->nextType = LEAFCHUNK_LEVEL;
            }
        }
        cblist.writelock[InsertINFO.start_point].unlock();
    }
}


int main(int argc, char ** argv)
{
    if(argc<5)
    {
        cout<<"[efile] [edge_num] [node_num] [insert_rate] [coro_num]\n";
        exit(-1);
    }
    string GRAPH_DIR = argv[1];
    string GRAPH_INFO_DIR = argv[2];
    auto [V, E] = LoadEVFromInfoFIle(GRAPH_INFO_DIR);
    int insert_rate = atoi(argv[3]);
    int coro_num = atoi(argv[4]);
    uint32_t thread_num = 4;
    uint32_t all_coro_num = coro_num * thread_num;
    int preload_E = E * (100 - insert_rate) / 100;
    int remain_E = E - preload_E;

    CoroGraph::CBList G(V, preload_E, GRAPH_DIR, E, InsertTarget);
    printf("load graph finish\n");
    
    auto CoroIndex = SliceTaskForCoro(remain_E, all_coro_num);

    
    //==============================================================time_start====================
    auto start = std::chrono::steady_clock::now();

    std::cout<<thread_num<<std::endl;
    omp_set_num_threads(thread_num);

    // #pragma omp parallel for
    for(uint32_t tid=0;tid<thread_num;++tid)
    {
        vector<coroutine_handle<>> InsertTasks(coro_num);
        for(uint32_t i = 0; i<coro_num; ++i)
        {
            InsertTasks[i]=CoInsertGraph(G, CoroIndex[coro_num * tid + i], CoroIndex[coro_num * tid + i + 1]).get_handle();
        }
        // std::cout << tid << omp_get_thread_num() << " process: " << SFCresult[tid * coro_num] << "~" << SFCresult[tid * coro_num + coro_num] << std::endl;
        int finished = 0;
        int flag = 0;
        while(finished < InsertTasks.size())
        {
            for(auto &t : InsertTasks)
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

    auto end = std::chrono::steady_clock::now();
//==============================================================================time_stop===========================
    std::chrono::duration<double, std::micro> elapsed = end - start; // std::micro 表示以微秒为时间单位
    std::cout<< "time: "  << elapsed.count() << "us" << std::endl;
    return EXIT_SUCCESS;
}