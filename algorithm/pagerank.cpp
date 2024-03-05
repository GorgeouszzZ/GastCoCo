// #define PRINT_DEBUG_MESSAGES
#include "graph.hpp"
// #include "graph.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <chrono>

#include "../other/atomic.hpp"

#include <math.h>

const double d = (double)0.85;

void compute(Graph<Empty> * graph, int iterations) {
  auto start = std::chrono::steady_clock::now();

  auto vertices = graph->cblptr->NodeNum;

  double * curr = new double[vertices];
  double * next = new double[vertices];
  VertexSubset * active = new VertexSubset(vertices);
  active->fill();

  double delta = graph->process_vertices<double>(
    [&](VertexID vtx){
      curr[vtx] = (double)1;
      if (graph->cblptroutgoing->NodeList[vtx].NeighboorCnt>0) {
        curr[vtx] /= graph->cblptroutgoing->NodeList[vtx].NeighboorCnt;
      }
      return (double)1;
    },
    active
  );
  delta /= vertices;


  for (IndexType i_i=0;i_i<iterations;i_i++) {

    graph->fill_vertex_array(next, (double)0);
    graph->process_edges<int,double>(
      [](VertexID dst) {
        return 0;
      },
      [&](VertexID dst) {
        auto dst_neighbour = graph->cblptr->NodeList[dst];
        if(dst_neighbour.NeighboorCnt <= LV1CHUNK_NCNT)
        {
            for(IndexType dst_i=0;dst_i<dst_neighbour.NeighboorCnt;dst_i++)
            {
                VertexID src = dst_neighbour.Neighboor.nextLv1Chunk->NeighboorChunk[dst_i].dest;
                next[dst] += curr[src];
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
                    next[dst] += curr[src];
                }
                n_ptr = n_ptr.nextLeafChunk->nextPtr;
            }
        }
        return 1;
        // write_add(&next[dst], sum);

      },
      active
    );
    if (i_i==iterations-1) {
      delta = graph->process_vertices<double>(
        [&](VertexID vtx) {
          next[vtx] = 1 - d + d * next[vtx];
          return 0;
        },
        active
      );
    } else {
      delta = graph->process_vertices<double>(
        [&](VertexID vtx) {
          next[vtx] = 1 - d + d * next[vtx];
          if (graph->cblptroutgoing->NodeList[vtx].NeighboorCnt>0) {
            next[vtx] /= graph->cblptroutgoing->NodeList[vtx].NeighboorCnt;
            return fabs(next[vtx] - curr[vtx]) * graph->cblptroutgoing->NodeList[vtx].NeighboorCnt;
          }
          return fabs(next[vtx] - curr[vtx]);
        },
        active
      );
    }
    delta /= vertices;
    std::swap(curr, next);

    std::cout << i_i << ": " << curr[6] << std::endl;
  }

  auto end = std::chrono::steady_clock::now();
  std::chrono::duration<double, std::micro> elapsed = end - start; // std::micro 表示以微秒为时间单位
  std::cout<< "time: "  << elapsed.count() << "us" << std::endl;
  std::cout<<curr[6]<<std::endl;


  delete active;
}

int main(int argc, char ** argv) {

  if (argc<5) {
    printf("pagerank [file] [infofile] [thread] [iterations]\n");
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
