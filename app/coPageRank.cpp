#include "../CBList/graph_coro.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <chrono>

#include "../other/atomic.hpp"
#include "../other/prefetch.hpp"

#include <math.h>

using namespace GastCoCo;

const double d = (double)0.85;
thread_local tcalloc coroutine_allocator;
double * curr;
double * next;
generator<void> coro_pagerank_process_edge(const CBList* cbl, VertexID dst_start, VertexID dst_end)
{
  for(VertexID dst = dst_start; dst < dst_end; ++dst)
  {
    auto dst_neighbour = cbl->VertexTableIn[dst];
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
            prefetch_Chunk(n_ptr.nextLeafChunk);
            co_await std::suspend_always{};
            for(IndexType dst_i=0;dst_i<n_ptr.nextLeafChunk->count;dst_i++)
            {
                VertexID src = n_ptr.nextLeafChunk->NeighboorChunk[dst_i].dest;
                next[dst] += curr[src];
            }
            n_ptr = n_ptr.nextLeafChunk->nextPtr;
        }
    }
  }
}

void compute(Graph<Empty> * graph, int iterations) {
  auto start = std::chrono::steady_clock::now();

  auto vertices = graph->cblptr->VertexNum;

  curr = new double[vertices];
  next = new double[vertices];
  VertexSubset * active = new VertexSubset(vertices);
  active->fill();

  double delta = graph->process_vertices_all_graph<double>(
    [&](VertexID vtx){
      curr[vtx] = (double)1;
      if (graph->cblptr->VertexTableIn[vtx].NeighboorCnt>0) {
        curr[vtx] /= graph->cblptr->VertexTableIn[vtx].NeighboorCnt;
      }
      return (double)1;
    }
  );
  delta /= vertices;


  for (IndexType i_i=0;i_i<iterations;i_i++) {

    graph->fill_vertex_array(next, (double)0);
    graph->coro_process_edges_all_graph<int, double>(
      coro_pagerank_process_edge
    );
    if (i_i==iterations-1) {
      delta = graph->process_vertices_all_graph<double>(
        [&](VertexID vtx) {
          next[vtx] = 1 - d + d * next[vtx];
          return 0;
        }
      );
    } else {
      delta = graph->process_vertices_all_graph<double>(
        [&](VertexID vtx) {
          next[vtx] = 1 - d + d * next[vtx];
          if (graph->cblptr->VertexTableIn[vtx].NeighboorCnt>0) {
            next[vtx] /= graph->cblptr->VertexTableIn[vtx].NeighboorCnt;
            return fabs(next[vtx] - curr[vtx]) * graph->cblptr->VertexTableIn[vtx].NeighboorCnt;
          }
          return fabs(next[vtx] - curr[vtx]);
        }
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
    printf("pagerank [infofile] [thread] [coro] [iterations]\n");
    exit(-1);
  }

  Graph<Empty> * graph;
  graph = new Graph<Empty>();
  int threads = std::atoi(argv[2]);
  int coros = std::atoi(argv[3]);
  graph->load_directed(argv[1], threads, coros);
  int iterations = std::atoi(argv[4]);

  compute(graph, iterations);
//   for (int run=0;run<5;run++) {
//     compute(graph, iterations);
//   }

  delete graph;
  return 0;
}
