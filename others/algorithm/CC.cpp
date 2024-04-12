// #define PRINT_DEBUG_MESSAGES
#include "graph_coro.hpp"
// #include "graph.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <chrono>

#include "../other/atomic.hpp"

#include <math.h>


void compute(Graph<Empty> * graph) {
  auto start = std::chrono::steady_clock::now();

  auto vertices = graph->cblptr->NodeNum;

  VertexID * label = new VertexID[vertices];
  VertexSubset * active_in = new VertexSubset(vertices);
  VertexSubset * active_out = new VertexSubset(vertices);
  active_in->fill();

  VertexID active_vertices = graph->process_vertices_all_graph<VertexID>(
    [&](VertexID vtx){
      label[vtx] = vtx;
      return 1;
    }
  );

  for (IndexType i_i=0;active_vertices>0;i_i++) {

    active_out->clear();
    active_vertices = graph->process_edges<VertexID,VertexID>(
      [&](VertexID src){
        VertexID activated = 0;
        auto src_neighbour = graph->cblptroutgoing->NodeList[src];
        if(src_neighbour.NeighboorCnt <= LV1CHUNK_NCNT)
        {
            for(IndexType src_i=0;src_i<src_neighbour.NeighboorCnt;src_i++)
            {
                VertexID dst = src_neighbour.Neighboor.nextLv1Chunk->NeighboorChunk[src_i].dest;
                if (label[src] < label[dst]) {
                    write_min(&label[dst], label[src]);
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
                    if (label[src] < label[dst]) {
                        write_min(&label[dst], label[src]);
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
        auto dst_neighbour = graph->cblptr->NodeList[dst];
        VertexID flag = 0;
        if(dst_neighbour.NeighboorCnt <= LV1CHUNK_NCNT)
        {
            for(IndexType dst_i=0;dst_i<dst_neighbour.NeighboorCnt;dst_i++)
            {
                VertexID src = dst_neighbour.Neighboor.nextLv1Chunk->NeighboorChunk[dst_i].dest;
                if (label[src] < label[dst])
                {
                    label[dst] = label[src];
                    flag = 1;
                    active_out->set_bit(dst);
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
                    VertexID src = n_ptr.nextLeafChunk->NeighboorChunk[dst_i].dest;
                    if (label[src] < label[dst])
                    {
                        label[dst] = label[src];
                        flag = 1;
                        active_out->set_bit(dst);
                    }
                }
                n_ptr = n_ptr.nextLeafChunk->nextPtr;
            }
        }
        return flag;
      },
      active_in
    );
    std::swap(active_in, active_out);

    // std::cout << i_i << ": " << curr[6] << std::endl;
  }

  auto end = std::chrono::steady_clock::now();
  std::chrono::duration<double, std::micro> elapsed = end - start; // std::micro 表示以微秒为时间单位
  std::cout<< "time: "  << elapsed.count() << "us" << std::endl;

    VertexID * count = new VertexID[vertices];
    graph->fill_vertex_array(count, 0u);
    for (VertexID v_i=0;v_i<vertices;v_i++) {
        count[label[v_i]] += 1;
    }
    VertexID components = 0;
    for (VertexID v_i=0;v_i<vertices;v_i++) {
        if (count[v_i] > 0) {
        components += 1;
        }
    }
    printf("components = %u\n", components);



  delete active_in;
  delete active_out;
}

int main(int argc, char ** argv) {

  if (argc<4) {
    printf("pagerank [file] [infofile] [thread]\n");
    exit(-1);
  }

  Graph<Empty> * graph;
  graph = new Graph<Empty>();
  int threads = std::atoi(argv[3]);
  graph->load_directed(argv[1], argv[2], threads);

  compute(graph);
//   for (int run=0;run<5;run++) {
//     compute(graph, iterations);
//   }

  delete graph;
  return 0;
}
