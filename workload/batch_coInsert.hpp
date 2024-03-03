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


generator<void> CoInsertGraph(const CoroGraph::CBList& cbl, const vector<Gedge_noWeight>& EdgeList, IndexType insert_start, IndexType insert_end)
{
    // co_await std::suspend_always{};
    IndexType processing_index = insert_start;
    while(processing_index < insert_end)
    {
        // if(processing_index >= insert_end)
        // {
        //     cout << "gogogo!@\n";
        //     break;
        // }
        // cout << "im " << insert_start << "--" << insert_end << "    " << processing_index << endl;
        auto InsertINFO = EdgeList[processing_index];
        ++processing_index;
        AdjUnit InsertINFO_adj = { InsertINFO.end_point, 0 };
        int flag = 0;
        if(cbl.NodeList[InsertINFO.start_point].NeighboorCnt < LV1CHUNK_NCNT)
        {
            cbl.NodeList[InsertINFO.start_point].Neighboor.nextLv1Chunk->NeighboorChunk[cbl.NodeList[InsertINFO.start_point].Neighboor.nextLv1Chunk->count] = InsertINFO_adj;
            ++cbl.NodeList[InsertINFO.start_point].Neighboor.nextLv1Chunk->count;
            ++cbl.NodeList[InsertINFO.start_point].NeighboorCnt;
            flag = 1;
        }
        else if(cbl.NodeList[InsertINFO.start_point].NeighboorCnt == LV1CHUNK_NCNT)
        {
            ++cbl.NodeList[InsertINFO.start_point].Level;
            for(IndexType i=0;i<cbl.NodeList[InsertINFO.start_point].NeighboorCnt;++i)
            {
                cbl.NodeList[InsertINFO.start_point].BPT.Insert(cbl.NodeList[InsertINFO.start_point].Neighboor.nextLv1Chunk->NeighboorChunk[i]);
            }
            cbl.NodeList[InsertINFO.start_point].BPT.Insert({InsertINFO.end_point, 0});
            ++cbl.NodeList[InsertINFO.start_point].NeighboorCnt;
            auto LastLeaf = cbl.NodeList[InsertINFO.start_point].BPT.GetLeafEnd();
            LastLeaf->nextType = cbl.NodeList[InsertINFO.start_point].Neighboor.nextLv1Chunk->nextType;
            LastLeaf->nextPtr = cbl.NodeList[InsertINFO.start_point].Neighboor.nextLv1Chunk->nextPtr;
            // delete cbl.NodeList[InsertINFO.start_point].Neighboor.nextLv1Chunk;
            cbl.NodeList[InsertINFO.start_point].Neighboor.nextLeafChunk = cbl.NodeList[InsertINFO.start_point].BPT.GetLeafStart();
            cbl.NodeList[InsertINFO.start_point].ChunkCnt = cbl.NodeList[InsertINFO.start_point].BPT.LeafCnt;
            flag = 2;
        }
        else
        {
            cbl.NodeList[InsertINFO.start_point].NeighboorCnt++;
            IndexType kk_forfind = 0;
            CoroGraph::Node* p = cbl.NodeList[InsertINFO.start_point].BPT.Root;
            CoroGraph::InterNode* q;
            while (p != nullptr)
            {
                // prefetch_Chunk(p);
                co_await std::suspend_always{};
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
            cbl.NodeList[InsertINFO.start_point].BPT.InsertKnowPos(InsertINFO_adj, InsertPos);
            cbl.NodeList[InsertINFO.start_point].ChunkCnt = cbl.NodeList[InsertINFO.start_point].BPT.LeafCnt;
            flag = 1;
        }
        if(flag == 0){ printf("error"); exit(-1); }
        int src = InsertINFO.start_point;
        if(flag == 2 && src != 0)
        {
            int level = cbl.NodeList[src-1].Level;
            if(level == 1)
            {
                cbl.NodeList[src-1].Neighboor.nextLv1Chunk->nextPtr = cbl.NodeList[src].Neighboor;
                cbl.NodeList[src-1].Neighboor.nextLv1Chunk->nextType = LEAFCHUNK_LEVEL;
            }
            else if(level == 2)
            {
                cbl.NodeList[src-1].BPT.GetLeafEnd()->nextPtr = cbl.NodeList[src].Neighboor;
                cbl.NodeList[src-1].BPT.GetLeafEnd()->nextType = LEAFCHUNK_LEVEL;
            }
        }

        // co_await std::suspend_always{};
    }
}
