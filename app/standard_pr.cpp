#include "bind_cpu.hpp"
#include "cblist.hpp"
#include <iostream>
#include <stdio.h>
#include <chrono>
#include <stack>
#include <omp.h>

using namespace std;
double d = 0.85;

void pagerank(const GastCoCo::CBList& cbl, vector<double>& node_state_old, vector<double>& node_state_new, int thread_num, int iter) {
    omp_set_num_threads(thread_num);
    vector<double> node_contrib(cbl.VertexNum, 0);
    vector<double> node_out_degree_inv(cbl.VertexNum, 0);

#pragma omp parallel for
    for (GastCoCo::VertexID src = 0; src < cbl.VertexNum; ++src) {
        auto out_degree = cbl.VertexTableOut[src].NeighboorCnt;
        node_out_degree_inv[src] = out_degree > 0 ? 1.0 / out_degree : 0;
    }

    while(iter) {
#pragma omp parallel for
        for (GastCoCo::VertexID src = 0; src < cbl.VertexNum; ++src) {
            node_contrib[src] = node_state_old[src] * node_out_degree_inv[src];
        }

    #pragma omp parallel for
        for(GastCoCo::VertexID dst = 0; dst < cbl.VertexNum; ++dst) {
            double sum = 0;
            if(cbl.VertexTableIn[dst].Level == 1) {
                auto chunk = cbl.VertexTableIn[dst].Neighboor.nextLv1Chunk;
                for(int i = 0; i < chunk->count; ++i) {
                    auto src = chunk->NeighboorChunk[i].dest;
                    sum += node_contrib[src];
                }
            }
            else {
                auto next_ptr = cbl.VertexTableIn[dst].Neighboor;
                for(int chunk_i = 0; chunk_i < cbl.VertexTableIn[dst].ChunkCnt; ++chunk_i) {
                    auto chunk = next_ptr.nextLeafChunk;
                    for(int i = 0; i < chunk->count; ++i) {
                        auto src = chunk->NeighboorChunk[i].dest;
                        sum += node_contrib[src];
                    }
                    next_ptr = chunk->nextPtr;
                }
            }
            node_state_new[dst] += sum * d;
        }

        swap(node_state_old, node_state_new);

    #pragma omp parallel for
        for(GastCoCo::VertexID i = 0; i < cbl.VertexNum; ++i) {
            node_state_new[i] = 1-d;
        }
        iter--;
        //getchar();

    }
}

int main(int argc, char ** argv) {
    if(argc<4) {
        cout<<"[efile] [thread] [iter]\n";
        exit(-1);
    }
    string GRAPH_DIR = argv[1];
    int thread_num = atoi(argv[2]);
    int iter = atoi(argv[3]);
    // BindCpu(2);
    GastCoCo::CBList test1(GRAPH_DIR, GastCoCo::GraphMode::Mixed, false);
    
    vector<double> node_state_old(test1.VertexNum, (double)1);
    vector<double> node_state_new(test1.VertexNum, 1-d);

//==============================================================time_start====================
    auto start = std::chrono::steady_clock::now();//if compute new coroutine spending time?

    pagerank(test1, node_state_old, node_state_new, thread_num, iter);

    auto end = std::chrono::steady_clock::now();
//==============================================================================time_stop===========================
    std::chrono::duration<double, std::micro> elapsed = end - start; // std::micro 表示以微秒为时间单位
    std::cout<< "time: "  << elapsed.count() << "us" << std::endl;
    std::cout<<node_state_old[6]<<std::endl;
    return EXIT_SUCCESS;
}
