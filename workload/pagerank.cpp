#include "../Bind_cpu.hpp"
#include "CBList.hpp"
#include "../Initalize_container.hpp"
#include <iostream>
#include <stdio.h>
#include <chrono>
#include <stack>

using namespace std;
double d = 0.85;

void pagerank(CBList cbl, vector<double>& node_state_old, vector<double>& node_state_new, int iter)
{
    while(iter)
    {
        int now_node = 0;
        int nextFlag = 0;
        if(cbl.NodeList[0].Level == 1) nextFlag = CHUNK_LEVEL;
        else if(cbl.NodeList[0].Level == 2) nextFlag = LEAFCHUNK_LEVEL;
        auto nextPtr_tmp = &(cbl.NodeList[0].Neighboor);
        int outDegree = cbl.NodeList[0].NeighboorCnt;
        double tmpPR = node_state_old[0] / outDegree;
        while(now_node != cbl.Node_num)
        {         
            if(nextFlag%2 != 0)
            {
                for(int i=0;i<nextPtr_tmp->nextLv1Chunk->count;i++)
                { 
                    node_state_new[nextPtr_tmp->nextLv1Chunk->NeighboorChunk[i].dest] += tmpPR * d;
                    // if(nextPtr_tmp->nextLv1Chunk->NeighboorChunk[i].dest == 0)
                    // {
                    //     printf("pr:%f + (%d)->tmp:%f\n",node_state_new[0],now_node,tmpPR);
                    // }
                }
                nextFlag = nextPtr_tmp->nextLv1Chunk->nextType;
                nextPtr_tmp = &(nextPtr_tmp->nextLv1Chunk->nextPtr);                 
            }
            else
            {
                for(int i=0;i<nextPtr_tmp->nextLeafChunk->count;i++)
                { 
                    node_state_new[nextPtr_tmp->nextLeafChunk->NeighboorChunk[i].dest] += tmpPR * d;
                    // if(nextPtr_tmp->nextLeafChunk->NeighboorChunk[i].dest == 0)
                    // {
                    //     printf("pr:%f + (%d)->tmp:%f\n",node_state_new[0],now_node,tmpPR);
                    // }
                }
                nextFlag = nextPtr_tmp->nextLeafChunk->nextType;
                nextPtr_tmp = &(nextPtr_tmp->nextLeafChunk->nextPtr);
            }
            if(nextFlag<0)
            {
                now_node++;
                outDegree = cbl.NodeList[now_node].NeighboorCnt;
                tmpPR = node_state_old[now_node] / outDegree;
            }
        }
        for(int i=0;i<node_state_old.size();i++)
        {
            // if(i < 5)
            //     printf("%f ",node_state_new[i]);
            double new_old = abs(node_state_new[i] - node_state_old[i]);
            // if(abs(page.newPR - page.oldPR) > threshold) //可以在这里设置阈值判断收敛
            //     shouldStop = 0;
            node_state_old[i] = node_state_new[i];
            node_state_new[i] = 1-d;
        }
        iter--;
        //getchar();

        cout<<"remain:"<<iter<<endl;
    }
}

int main(int argc, char ** argv)
{
    if(argc<5)
    {
        cout<<"[efile] [edge_num] [node_num] [iter]\n";
        exit(-1);
    }
    string GRAPH_DIR = argv[1];
    int E = atoi(argv[2]);
    int V = atoi(argv[3]);
    int iter = atoi(argv[4]);
    BindCpu(2);
    CBList test1(V, E, GRAPH_DIR);
    
    vector<double> node_state_old(test1.Node_num);
    vector<double> node_state_new(test1.Node_num);
    init_vec(node_state_old, (double)1);
    init_vec(node_state_new, 1-d);

//==============================================================time_start====================
    auto start = std::chrono::steady_clock::now();//if compute new coroutine spending time?

    pagerank(test1, node_state_old, node_state_new, iter);

    auto end = std::chrono::steady_clock::now();
//==============================================================================time_stop===========================
    std::chrono::duration<double, std::micro> elapsed = end - start; // std::micro 表示以微秒为时间单位
    std::cout<< "time: "  << elapsed.count() << "us" << std::endl;
    std::cout<<node_state_old[6]<<std::endl;
    return EXIT_SUCCESS;
}