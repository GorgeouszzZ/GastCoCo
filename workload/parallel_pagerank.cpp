#include "../other/Bind_cpu.hpp"
#include "../CBList/CBList.hpp"
#include <iostream>
#include <stdio.h>
#include <chrono>
#include <stack>
#include "omp.h"
#include "../other/atomic.hpp"
#include "Slice_for_CBL_coro.hpp"

using namespace std;
double d = 0.85;

int kk = 784995;
uint32_t thread_num = 0;

void pagerank(const CoroGraph::CBList& cbl, const VertexID& left, const VertexID& right, vector<double>& node_state_old, vector<double>& node_state_new, int& clean_flag, int32_t iter)
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
        // int Lv1cnt = 0;
        // int Leafcnt = 0;
        // auto startall = std::chrono::steady_clock::now();
        // //-----profiling-----
        // std::cout<<"1sum="<<node_state_new[kk]<<" \n";
        while(now_node != right)
        {
            if(nextFlag%2 != 0)
            {
                // //-----profiling-----
                // ++Lv1cnt;
                // auto startLv1 = std::chrono::steady_clock::now();
                // //-----profiling-----
                for(int i=0;i<nextPtr_tmp->nextLv1Chunk->count;i++)
                { 
                    // if(nextPtr_tmp->nextLv1Chunk->NeighboorChunk[i].dest == kk && now_node == 785031) 
                    //     cout<<"nn:"<<now_node<<"old:"<<node_state_old[now_node]<<" delta:"<<tmpPR<<endl;
                        // cout<<tmpPR<<endl;
                        //cout<<delta<<endl;
                    write_add(&node_state_new[nextPtr_tmp->nextLv1Chunk->NeighboorChunk[i].dest], tmpPR);
                    // if(nextPtr_tmp->nextLv1Chunk->NeighboorChunk[i].dest == 0)
                    //     cout<<"back-"<<node_state_new[nextPtr_tmp->nextLv1Chunk->NeighboorChunk[i].dest]<<endl;
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
                // ++Leafcnt;
                // auto startLeaf = std::chrono::steady_clock::now();
                // //-----profiling-----
                for(int i=0;i<nextPtr_tmp->nextLeafChunk->count;i++)
                { 
                    // // double delta = tmpPR * d;
                    // if(nextPtr_tmp->nextLeafChunk->NeighboorChunk[i].dest == kk && now_node == 785031) 
                    //     cout<<"nn:"<<now_node<<"old:"<<node_state_old[now_node]<<" delta:"<<tmpPR<<endl;
                        // cout<<"nn:"<<now_node<<"old:"<<node_state_old[now_node]<<" delta:"<<tmpPR<<endl;
                        // cout<<tmpPR<<endl;
                        //cout<<delta<<endl;
                    write_add(&node_state_new[nextPtr_tmp->nextLeafChunk->NeighboorChunk[i].dest], tmpPR);
                    // if(nextPtr_tmp->nextLeafChunk->NeighboorChunk[i].dest == kk)
                        // cout<<"back-"<<node_state_new[nextPtr_tmp->nextLeafChunk->NeighboorChunk[i].dest]<<endl;
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
                // Leaftime += elapsedLeaf.count();
                // //-----profiling-----
            }
            if(nextFlag<0)
            {
                ++now_node;
                outDegree = cbl.NodeList[now_node].NeighboorCnt;
                tmpPR = node_state_old[now_node] / outDegree;
            }
            // printf("%d %d\n", omp_get_thread_num(), iter);
        }
        // //-----profiling-----
        // auto endall = std::chrono::steady_clock::now(); 
        // std::chrono::duration<double, std::micro> elapsedall = endall - startall;
        // std::cout<<"lv1cnt: "<<Lv1cnt<<" leafcnt: "<<Leafcnt<<" lv1time: "<<Lv1time<<" leaftime: "<<Leaftime<<std::endl;
        // std::cout<<"alltime: "<<elapsedall.count()<<std::endl;
        // //-----profiling-----
        __sync_add_and_fetch(&clean_flag, 1);
        // cout<<" remain:"<<omp_get_thread_num()<<" "<<clean_flag<<" "<<iter<<endl;
        if(clean_flag == thread_num)
        { 
            for(int i=0;i<node_state_old.size();i++)
            {
                // double new_old = abs(node_state_new[i] - node_state_old[i]);
                // if(abs(page.newPR - page.oldPR) > threshold) //可以在这里设置阈值判断收敛
                //     shouldStop = 0;
                // if(i==kk) std::cout<<"sum="<<node_state_new[i]<<" \n";
                // if(i==785031) std::cout<<"sum="<<node_state_new[i]<<" \n";
                node_state_old[i] = node_state_new[i] * d + 1-d;
                node_state_new[i] = 0;
                // if(i==kk) std::cout<<node_state_old[i]<<" \n";
                // if(i==785031) std::cout<<node_state_old[i]<<" \n";
            }
            clean_flag = 0;
        }
        while(clean_flag > 0 && iter!=0) {};//同步屏障
        // cout<<"remain:"<<iter<<endl;
    }

}

int main(int argc, char ** argv)
{
    if(argc<4)
    {
        cout<<"[efile] [edge_num] [node_num] [iter]\n";
        exit(-1);
    }
    string GRAPH_DIR = argv[1];
    string GRAPH_INFO_DIR = argv[2];
    auto [V, E] = LoadEVFromInfoFile(GRAPH_INFO_DIR);
    int iter = atoi(argv[3]);
    thread_num = atoi(argv[4]);
    // iter *= thread_num;
    CoroGraph::CBList test1(V, E, GRAPH_DIR);
    auto SFCresult = SliceForCoroDynamic(test1, thread_num);
    
    vector<double> node_state_old(test1.NodeNum, (double)1);
    vector<double> node_state_new(test1.NodeNum, 0);

//==============================================================time_start====================
    auto start = std::chrono::steady_clock::now();

    std::cout<<thread_num<<std::endl;
    omp_set_num_threads(thread_num);

    int clean_flag = 0;
    #pragma omp parallel for
    for(uint32_t tid=0; tid<thread_num; ++tid)
    {
        pagerank(test1, SFCresult[tid], SFCresult[tid+1], node_state_old, node_state_new, clean_flag, iter);
    }

    auto end = std::chrono::steady_clock::now();
//==============================================================================time_stop===========================
    std::chrono::duration<double, std::micro> elapsed = end - start; // std::micro 表示以微秒为时间单位
    std::cout<< "time: "  << elapsed.count() << "us" << std::endl;
    std::cout<<node_state_old[6]<<std::endl;
    return EXIT_SUCCESS;
}