
#define PRINT_DEBUG_MESSAGES
#include <stdio.h>
#include <stdlib.h>
#include <chrono>

#include "graph.hpp"
#include "../other/atomic.hpp"
#include <omp.h>


typedef int Weight;
std::vector<std::pair<int, int>> record;

void compute(Graph<Empty> * graph, VertexID root) {

  VertexID vertices = graph->cblptr->NodeNum;    
  VertexID * parent = new VertexID[vertices];
  VertexSubset * visited = new VertexSubset(vertices);
  VertexSubset * active_in = new VertexSubset(vertices);
  VertexSubset * active_out = new VertexSubset(vertices);

  visited->clear();
  visited->set_bit(root);
  active_in->clear();
  active_in->set_bit(root);
  graph->fill_vertex_array(parent, vertices);
  parent[root] = root;

  VertexID active_vertices = 1;
 auto start = std::chrono::steady_clock::now();

  for (IndexType i_i=0;active_vertices>0;++i_i) {
    active_out->clear();
    active_vertices = graph->process_edges<VertexID, VertexID>(
      [&](VertexID src){
        VertexID activated = 0;
        auto src_neighbour = graph->cblptroutgoing->NodeList[src];
        if(src_neighbour.NeighboorCnt <= LV1CHUNK_NCNT)
        {
            for(IndexType src_i=0;src_i<src_neighbour.NeighboorCnt;src_i++)
            {
                VertexID dst = src_neighbour.Neighboor.nextLv1Chunk->NeighboorChunk[src_i].dest;
                if (parent[dst]==vertices && cas(&parent[dst], vertices, src)) {
                    active_out->set_bit(dst);
                    activated += 1;
                }
            }
        }
        else
        {
            auto n_ptr = src_neighbour.Neighboor;
            for(IndexType loop_i=0;loop_i<src_neighbour.ChunkCnt;loop_i++)
            {
                for(IndexType src_i=0;src_i<n_ptr.nextLeafChunk->count;src_i++)
                {
                    VertexID dst = n_ptr.nextLeafChunk->NeighboorChunk[src_i].dest;
                    if (parent[dst]==vertices && cas(&parent[dst], vertices, src)) {
                        active_out->set_bit(dst);
                        activated += 1;
                    }
                }
                n_ptr = n_ptr.nextLeafChunk->nextPtr;
            }
        }
        return activated;
      },
      [&](VertexID dst) {
        if (visited->get_bit(dst)) return 0;
        auto dst_neighbour = graph->cblptr->NodeList[dst];
        if(dst_neighbour.NeighboorCnt <= LV1CHUNK_NCNT)
        {
            for(IndexType dst_i=0;dst_i<dst_neighbour.NeighboorCnt;dst_i++)
            {
                VertexID src = dst_neighbour.Neighboor.nextLv1Chunk->NeighboorChunk[dst_i].dest;
                if (active_in->get_bit(src) && cas(&parent[dst], vertices, src)) {
                    active_out->set_bit(dst);
                    return 1;
                }
            }
        }
        else
        {
            auto n_ptr = dst_neighbour.Neighboor;
            for(IndexType loop_i=0;loop_i<dst_neighbour.ChunkCnt;loop_i++)
            {
                for(IndexType dst_i=0;dst_i<n_ptr.nextLeafChunk->count;dst_i++)
                {
                    VertexID src = dst_neighbour.Neighboor.nextLeafChunk->NeighboorChunk[dst_i].dest;
                    if (active_in->get_bit(src) && cas(&parent[dst], vertices, src)) {
                        active_out->set_bit(dst);
                        return 1;
                    }
                }
                n_ptr = n_ptr.nextLeafChunk->nextPtr;
            }
        }
        return 0;
      },
      active_in
    );
    active_vertices = graph->process_vertices<VertexID>(
      [&](VertexID vtx) {
        visited->set_bit(vtx);
        return 1;
      },
      active_out
    );
    std::swap(active_in, active_out);
  }
    auto end = std::chrono::steady_clock::now();
  std::chrono::duration<double, std::micro> elapsed = end - start; // std::micro 表示以微秒为时间单位
  std::cout<< "time: "  << elapsed.count() << "us" << std::endl;
  

  delete active_in;
  delete active_out;
  delete visited;
}

int main(int argc, char ** argv) {

  if (argc<4) {
    printf("bfs [file] [infofile] [thread] [root]\n");
    exit(-1);
  }

  Graph<Empty> * graph;
  graph = new Graph<Empty>();
  graph->load_directed(argv[1], argv[2], atoi(argv[3]));
  VertexID root = std::atoi(argv[3]);
    
    compute(graph, root);

  delete graph;
  return 0;
}
