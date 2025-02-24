#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <stdio.h>
#include <coroutine>
#include <stack>
#include "../CBList/CBList.hpp"
#include "Coro_for_graph_v2.hpp"
#include "../other/prefetch.hpp"
#include "../other/atomic.hpp"
#include <mutex>
using namespace std;
double d = 0.85;

std::mutex mtx;

template <typename ptrType>
void compute_pr_OPT(ptrType TMPptr, double tmpPR, vector<double> &vertex_state_new)
{
    for (int i = 0; i < TMPptr->count; i++)
    {
        double delta = tmpPR * d;
        write_add(&vertex_state_new[TMPptr->NeighboorChunk[i].dest], delta);
    }
}

generator<void> pagerank_one_iter(const GastCoCo::CBList &cbl, const GastCoCo::VertexID &left, const GastCoCo::VertexID &right, vector<double> &vertex_state_old, vector<double> &vertex_state_new, bool pre_flag)
{
    GastCoCo::VertexID now_vertex = left;
    int nextFlag = 0;
    if (cbl.VertexTableIn[left].Level == 1)
        nextFlag = CHUNK_LEVEL;
    else if (cbl.VertexTableIn[left].Level == 2)
        nextFlag = LEAFCHUNK_LEVEL;
    auto nextPtr_tmp = &(cbl.VertexTableIn[left].Neighboor);
    int outDegree = cbl.VertexTableIn[left].NeighboorCnt;
    double tmpPR = vertex_state_old[left] / outDegree;

    // //-----profiling-----
    // double Lv1time = 0.0;
    // double Leaftime = 0.0;
    // double Leaftime2 = 0.0;
    // int Lv1cnt = 0;
    // int Leafcnt = 0;
    // auto startall = std::chrono::steady_clock::now();
    // //-----profiling-----

    while (now_vertex != right)
    {
        if (nextFlag % 2 != 0)
        {

            // //-----profiling-----
            // ++Lv1cnt;
            // auto startLv1 = std::chrono::steady_clock::now();
            // //-----profiling-----

            GastCoCo::prefetch_Chunk(nextPtr_tmp->nextLv1Chunk);
            co_await suspend_always{};
            compute_pr_OPT(nextPtr_tmp->nextLv1Chunk, tmpPR, vertex_state_new);
            // for(int i=0;i<nextPtr_tmp->nextLv1Chunk->count;i++)
            // {
            //     double delta = tmpPR * d;
            //     write_add(&vertex_state_new[nextPtr_tmp->nextLv1Chunk->NeighboorChunk[i].dest], delta);
            //     // if(nextPtr_tmp->nextLv1Chunk->NeighboorChunk[i].dest == 0)
            //     // {
            //     //     printf("pr:%f + (%d)->tmp:%f   ",vertex_state_new[0],now_vertex,tmpPR);
            //     //     printf("from %d-%d\n",left,right);
            //     // }
            // }
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

            GastCoCo::prefetch_TreeNode(nextPtr_tmp->nextLeafChunk);
            co_await suspend_always{};

            // //-----profiling-----
            // ++Leafcnt;
            // auto startLeaf = std::chrono::steady_clock::now();
            // //-----profiling-----

            compute_pr_OPT(nextPtr_tmp->nextLeafChunk, tmpPR, vertex_state_new);

            // for(int i=0;i<nextPtr_tmp->nextLeafChunk->count;i++)
            // {
            //     double delta = tmpPR * d;
            //     write_add(&vertex_state_new[nextPtr_tmp->nextLeafChunk->NeighboorChunk[i].dest], delta);
            //     // if(nextPtr_tmp->nextLeafChunk->NeighboorChunk[i].dest == 0)
            //     // {
            //     //     printf("pr:%f + (%d)->tmp:%f",vertex_state_new[0],now_vertex,tmpPR);
            //     //     printf("from %d-%d\n",left,right);
            //     // }
            // }
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
        if (nextFlag < 0)
        {
            ++now_vertex;
            outDegree = cbl.VertexTableIn[now_vertex].NeighboorCnt;
            tmpPR = vertex_state_old[now_vertex] / outDegree;
        }
    }
    // if(false) co_await suspend_always{};
}