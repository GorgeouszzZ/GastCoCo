#include "graph_coro.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <chrono>

#include "../other/atomic.hpp"

#include <math.h>

#define LABEL 15

void compute(Graph<Empty> * graph, int iterations) {
  auto start = std::chrono::steady_clock::now();

  auto vertices = graph->cblptr->NodeNum;

  auto **lb_cnt = new uint32_t*[LABEL+1];
  for (uint32_t i = 0; i < LABEL+1; ++i) {
    *(lb_cnt + i) = new uint32_t[vertices];
  }

  auto *lb = new uint32_t[vertices];
  #pragma omp parallel for
  for (uint32_t i = 0; i < vertices; ++i) {
    lb[i] = (i%LABEL+LABEL)%LABEL + 1;
  }


  for (IndexType i_i=0;i_i<iterations;i_i++) {

    graph->process_edges_all_graph<int,double>(
      [&](VertexID dst){
        auto dst_neighbour = graph->cblptr->NodeList[dst];
        if(dst_neighbour.NeighboorCnt <= LV1CHUNK_NCNT)
        {
            for(IndexType dst_i=0;dst_i<dst_neighbour.NeighboorCnt;dst_i++)
            {
                VertexID src = dst_neighbour.Neighboor.nextLv1Chunk->NeighboorChunk[dst_i].dest;
                ++lb_cnt[lb[dst]][src];
            }
        }
        else
        {
            auto n_ptr = dst_neighbour.Neighboor;
            for(IndexType loop_i=0;loop_i<dst_neighbour.ChunkCnt;loop_i++)
            {
                for(IndexType dst_i=0;dst_i<n_ptr.nextLeafChunk->count;dst_i++)
                {
                    VertexID src = n_ptr.nextLeafChunk->NeighboorChunk[dst_i].dest;
                    ++lb_cnt[lb[dst]][src];
                }
                n_ptr = n_ptr.nextLeafChunk->nextPtr;
            }
        }
        return 1;
      }
    );
    graph->process_vertices_all_graph<double>(
        [&](VertexID vtx) {
            if(vtx == 0) return 1;
            uint32_t max_lb = 0;
            uint32_t max_cnt = 0;
            for(uint32_t ilb = LABEL; ilb>=1 ; --ilb){
                if(max_cnt <= lb_cnt[ilb][vtx]){
                    max_cnt = lb_cnt[ilb][vtx];
                    max_lb = ilb;
                }
            }
            lb[vtx] = max_lb;
            return 1;
        }
    );

    graph->process_vertices_all_graph<double>(
        [&](VertexID vtx) {
            for(uint32_t ilb =1 ; ilb<=LABEL ; ++ilb){
            lb_cnt[ilb][vtx] = 0;
            }
            return 1;
        }
    );
    //or
    // for(uint32_t ilb = 1; ilb<=LABEL; ++ilb)
    //     graph->fill_vertex_array(lb_cnt[ilb], uint32_t(0));
  }

  auto end = std::chrono::steady_clock::now();
  std::chrono::duration<double, std::micro> elapsed = end - start; // std::micro 表示以微秒为时间单位
  std::cout<< "time: "  << elapsed.count() << "us" << std::endl;
  std::cout<<lb[6]<<std::endl;

}

int main(int argc, char ** argv) {

  if (argc<5) {
    printf("LP [file] [infofile] [thread]  [iterations]\n");
    exit(-1);
  }

  Graph<Empty> * graph;
  graph = new Graph<Empty>();
  int threads = std::atoi(argv[3]);
  graph->load_directed(argv[1], argv[2], threads);
  int iterations = std::atoi(argv[4]);

  compute(graph, iterations);
//   for (int run=0;run<5;run++) {
//     compute(graph, iterations);
//   }

  delete graph;
  return 0;
}
