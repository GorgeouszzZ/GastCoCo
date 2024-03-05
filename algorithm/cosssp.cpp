#include "graph_coro.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <chrono>

#include "../other/atomic.hpp"
#include "../other/prefetch.hpp"

#include <math.h>

typedef int Weight;
thread_local tcalloc coroutine_allocator;

Weight * distance;
VertexSubset * active_in;
VertexSubset * active_out;
generator<void> coro_sssp_sparse(const CoroGraph::CBList* cbl, VertexID src_start, VertexID src_end, int& activated)
{   
    for(VertexID src = src_start; src < src_end; ++src)
    {
        auto src_neighbour = cbl->NodeList[src];
        if(src_neighbour.NeighboorCnt <= LV1CHUNK_NCNT)
        {
            for(IndexType src_i=0;src_i<src_neighbour.NeighboorCnt;src_i++)
            {
                VertexID dst = src_neighbour.Neighboor.nextLv1Chunk->NeighboorChunk[src_i].dest;
                auto val = src_neighbour.Neighboor.nextLv1Chunk->NeighboorChunk[src_i].value;
                Weight relax_dist = distance[src] + val;
                if (relax_dist < distance[dst]) {
                    if (write_min(&distance[dst], relax_dist)) {
                        active_out->set_bit(dst);
                        activated += 1;
                    } 
                }
            }
        }
        else
        {
            auto n_ptr = src_neighbour.Neighboor;
            for(IndexType loop_i=0;loop_i<src_neighbour.ChunkCnt;loop_i++)
            {
                prefetch_Chunk(n_ptr.nextLeafChunk);
                co_await std::suspend_always{};
                for(IndexType src_i=0;src_i<n_ptr.nextLeafChunk->count;src_i++)
                {
                    VertexID dst = n_ptr.nextLeafChunk->NeighboorChunk[src_i].dest;
                    auto val = n_ptr.nextLeafChunk->NeighboorChunk[src_i].value;
                    Weight relax_dist = distance[src] + val;
                    if (relax_dist < distance[dst]) {
                        if (write_min(&distance[dst], relax_dist)) {
                            active_out->set_bit(dst);
                            activated += 1;
                        }
                    }
                }
                n_ptr = n_ptr.nextLeafChunk->nextPtr;
            }
        }
    }
}

generator<void> coro_sssp_dense(const CoroGraph::CBList* cbl, VertexID dst_start, VertexID dst_end, int& activated)
{   
    for(VertexID dst = dst_start; dst < dst_end; ++dst)
    {
        auto dst_neighbour = cbl->NodeList[dst];
        if(dst_neighbour.NeighboorCnt <= LV1CHUNK_NCNT)
        {
            for(IndexType dst_i=0;dst_i<dst_neighbour.NeighboorCnt;dst_i++)
            {
                VertexID src = dst_neighbour.Neighboor.nextLv1Chunk->NeighboorChunk[dst_i].dest;
                auto val = dst_neighbour.Neighboor.nextLv1Chunk->NeighboorChunk[dst_i].value;
                Weight relax_dist = distance[src] + val;
                if (relax_dist < distance[dst]) {
                    ++activated;
                    distance[dst] = relax_dist;
                    active_out->set_bit(dst);
                }
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
                    auto val = n_ptr.nextLeafChunk->NeighboorChunk[dst_i].value;
                    Weight relax_dist = distance[src] + val;
                    if (relax_dist < distance[dst]) {
                        ++activated;
                        distance[dst] = relax_dist;
                        active_out->set_bit(dst);
                    }
                }
                n_ptr = n_ptr.nextLeafChunk->nextPtr;
            }
        }
    }
}

void compute(Graph<Weight> * graph, VertexID root) {

  auto vertices = graph->cblptr->NodeNum;

  distance = new Weight[vertices];
  active_in = new VertexSubset(vertices);
  active_out = new VertexSubset(vertices);
  active_in->clear();
  active_in->set_bit(root);
  graph->fill_vertex_array(distance, (Weight)1e9);
  distance[root] = (Weight)0;
  VertexID active_vertices = 1;
  int i_i=0;
  auto start = std::chrono::steady_clock::now();

  for (;active_vertices>0;i_i++)  {

    active_out->clear();

    active_vertices = graph->coro_process_edges<int,double>(
      coro_sssp_sparse,
      coro_sssp_dense,
      active_in
    );
    std::swap(active_in, active_out);
  }
    std::cout<<"round: "<<i_i<<std::endl;
  auto end = std::chrono::steady_clock::now();
  std::chrono::duration<double, std::micro> elapsed = end - start; // std::micro 表示以微秒为时间单位
  std::cout<< "time: "  << elapsed.count() << "us" << std::endl;
  std::cout<<distance[6]<<std::endl;

}

int main(int argc, char ** argv) {

  if (argc<6) {
    printf("pagerank [file] [infofile] [thread] [coro] [root]\n");
    exit(-1);
  }

  Graph<Weight> * graph;
  graph = new Graph<Weight>();
  int threads = std::atoi(argv[3]);
  int coros = std::atoi(argv[4]);
  graph->load_directed(argv[1], argv[2], threads, coros);
  VertexID root = std::atoi(argv[5]);

  compute(graph, root);
//   for (int run=0;run<5;run++) {
//     compute(graph, iterations);
//   }

  delete graph;
  return 0;
}
