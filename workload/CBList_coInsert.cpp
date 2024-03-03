#include "../CBList/CBList.hpp"
#include <iostream>
#include <stdio.h>
#include <chrono>
#include <stack>
#include <queue>
#include <vector>
#include "../other/Coro_graph_v2.hpp"
#include "Slice_for_CBL_coro.hpp"

using namespace CoroGraph;

thread_local tcalloc coroutine_allocator;

#define GET_NUM 64

std::vector<Gedge> InsertTarget;

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

// generator<void> CoInsertToBplusTree(BplusTree bpt, AdjUnit NewNeighboor)
// {
//     int kk_forfind = 0;
//     Node* p = bpt.Root;
//     InterNode* q;
//     while (p != nullptr)
//     {
//         prefetch_Chunk(p);
//         co_await suspend_always{};
//         if (p->IsLeaf)
//             break;
//         for (kk_forfind = 0; kk_forfind < p->count; kk_forfind++)
//         {
//             if (NewNeighboor.dest < ((InterNode*)p)->KeyList[kk_forfind])
//                 break;
//         }
//         q = (InterNode*)p;
//         p = q->Child[kk_forfind];
//     }
//     LeafChunk* InsertPos = (LeafChunk*)p;
//     if(InsertPos == nullptr)
//     {
//         InsertPos = new LeafChunk();
//         bpt.Root = InsertPos;
//         bpt.LeafCnt++; 
//     }
//     if(InsertPos->count < LEAF_CHUNK_NCNT)
//         InsertPos->Insert(NewNeighboor);
//     else
//     {
//         LeafChunk * NewLeafChunk = new LeafChunk();
//         int NewLeafChunkFirstDest = InsertPos->Split(NewLeafChunk);
//         bpt.LeafCnt++;
//         if(NewNeighboor.dest < NewLeafChunkFirstDest)
//         {
//             InsertPos->Insert(NewNeighboor);
//         }
//         else
//         {
//             NewLeafChunk->Insert(NewNeighboor);
//         }
//         InterNode* this_parent = (InterNode*)(InsertPos->Parent);
//         if (nullptr == this_parent)//初始化parent，若没有父结点，新建一个
//         {
//             InterNode* New_Root = new InterNode;
//             New_Root->Child[0] = InsertPos;
//             New_Root->KeyList[0] = NewLeafChunkFirstDest;
//             New_Root->Child[1] = NewLeafChunk;
//             InsertPos->Parent = New_Root;
//             NewLeafChunk->Parent = New_Root;
//             New_Root->count = 1;
//             bpt.Root = New_Root;
//         }
//         else
//         {
//             auto addnode_Parent = this_parent;
//             auto addnode_NewKey = NewLeafChunkFirstDest;
//             Node* addnode_NewNode = NewLeafChunk;
//             while(nullptr != addnode_Parent && !addnode_Parent->IsLeaf)
//             {
//                 if (addnode_Parent->count < INTER_NODE_KEY_CNT)//父亲不满
//                 {
//                     addnode_Parent->Insert(addnode_NewKey, addnode_NewNode);
//                     break;
//                 }
//                 else
//                 {
//                     InterNode* Brother = new InterNode;
//                     //叶子节点满，父节点也满分裂情况
//                     int addnode_NewKey1 = addnode_NewKey;
//                     int InsertPos = addnode_Parent->Split(Brother, addnode_NewKey1);//NewKey为需要提取并插入到root节点的值

//                     //确定需要插入的关键字，是插入到分裂节点的哪个位置
//                     if(InsertPos)
//                         Brother->Insert(addnode_NewKey, addnode_NewNode);
//                     else
//                         addnode_Parent->Insert(addnode_NewKey, addnode_NewNode);


//                     InterNode* addnode_Pparent = (InterNode*)(addnode_Parent->Parent);
//                     if (NULL == addnode_Pparent)
//                     {
//                         addnode_Pparent = new InterNode();
//                         addnode_Pparent->Child[0] = addnode_Parent;
//                         addnode_Pparent->KeyList[0] = addnode_NewKey1;//newkey为分裂传回，为插入的中间值
//                         addnode_Pparent->Child[1] = Brother;
//                         addnode_Parent->Parent = addnode_Pparent;
//                         Brother->Parent = addnode_Pparent;
//                         addnode_Pparent->count = 1;
//                         bpt.Root = addnode_Pparent;
//                         break;
//                     }
//                     addnode_Parent = addnode_Pparent;
//                     addnode_NewKey = addnode_NewKey1;
//                     addnode_NewNode = Brother;
//                 }
//             }
//         }
//     }
// }

generator<void> CoInsertGraph(const CBList& cblist, EdgeID left, EdgeID right)
{
    int process_pos = left;
    while(process_pos != right)
    {
        int this_process = process_pos;
        process_pos++;
        auto InsertINFO = InsertTarget[this_process];
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
            // delete cblist.NodeList[InsertINFO.start_point].Neighboor.nextLv1Chunk;
            cblist.NodeList[InsertINFO.start_point].Neighboor.nextLeafChunk = cblist.NodeList[InsertINFO.start_point].BPT.GetLeafStart();
            cblist.NodeList[InsertINFO.start_point].ChunkCnt = cblist.NodeList[InsertINFO.start_point].BPT.LeafCnt;
            flag = 2;
        }
        else
        {
            cblist.NodeList[InsertINFO.start_point].NeighboorCnt++;
            int kk_forfind = 0;
            Node* p = cblist.NodeList[InsertINFO.start_point].BPT.Root;
            InterNode* q;
            while (p != nullptr)
            {
                // prefetch_Chunk(p);
                co_await std::suspend_always{};
                if (p->IsLeaf)
                    break;
                for (kk_forfind = 0; kk_forfind < p->count; kk_forfind++)
                {
                    if (InsertINFO.end_point < ((InterNode*)p)->KeyList[kk_forfind])
                        break;
                }
                q = (InterNode*)p;
                p = q->Child[kk_forfind];
            }
            LeafChunk* InsertPos = (LeafChunk*)p;
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
    }
}


int main(int argc, char ** argv)
{
    if(argc<5)
    {
        std::cout<<"[efile] [infofile] [insert_rate] [coro_num]\n";
        exit(-1);
    }
    std::string GRAPH_DIR = argv[1];
    std::string GRAPH_INFO_DIR = argv[2];
    auto [V, E] = LoadEVFromInfoFile(GRAPH_INFO_DIR);
    int insert_rate = atoi(argv[3]);
    int coro_num = atoi(argv[4]);

    int preload_E = E * (100 - insert_rate) / 100;
    int remain_E = E - preload_E;

    CoroGraph::CBList G(V, preload_E, GRAPH_DIR, E, InsertTarget);
    printf("load graph finish\n");
    
    auto CoroIndex = SliceTaskForCoro(remain_E, coro_num);
    
    std::vector<std::coroutine_handle<>> InsertTasks(coro_num);
    
    //==============================================================time_start====================
    auto start = std::chrono::steady_clock::now();//if compute new coroutine spending time?

    for(int i=0;i<coro_num;++i)
    {
        InsertTasks[i]=CoInsertGraph(G, CoroIndex[i], CoroIndex[i+1]).get_handle();
    }

    int finished = 0;
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

    auto end = std::chrono::steady_clock::now();
//==============================================================================time_stop===========================
    std::chrono::duration<double, std::micro> elapsed = end - start; // std::micro 表示以微秒为时间单位
    std::cout<< "time: "  << elapsed.count() << "us" << std::endl;

    return EXIT_SUCCESS;
}