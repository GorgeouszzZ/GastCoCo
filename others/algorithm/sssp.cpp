
// #define PRINT_DEBUG_MESSAGES
#include <stdio.h>
#include <stdlib.h>
#include <chrono>

#include "graph.hpp"
#include "../other/atomic.hpp"
#include <omp.h>


typedef int Weight;
std::vector<std::pair<int, int>> record;

void compute(Graph<Weight> * graph, VertexID root) {


  auto vertices = graph->cblptr->NodeNum;

  Weight * distance = new Weight[vertices];
  VertexSubset * active_in = new VertexSubset(vertices);
  VertexSubset * active_out = new VertexSubset(vertices);
  active_in->clear();
  active_in->set_bit(root);
  graph->fill_vertex_array(distance, (Weight)1e9);
  distance[root] = (Weight)0;
  VertexID active_vertices = 1;
  int i_i=0;
    auto start = std::chrono::steady_clock::now();
  for (;active_vertices>0;i_i++) {
    // if(i_i < 5)
    // {
    //     std::cout<<i_i<<": "<<distance[6] << " " << active_vertices << record.size() <<std::endl;
    //     // getchar();
    //     // for(auto& rec:record)
    //     //     std::cout<<rec.first<<"-"<<rec.second<<std::endl;
    //     record.clear();
    // }
    // std::cout<<"iter: "<<i_i << "  "<<active_vertices<<std::endl;
    active_out->clear();
    active_vertices = graph->process_edges<VertexID,Weight>(
      [&](VertexID src){
        VertexID activated = 0;
        auto src_neighbour = graph->cblptroutgoing->NodeList[src];
        if(src_neighbour.NeighboorCnt <= LV1CHUNK_NCNT)
        {
            for(IndexType src_i=0;src_i<src_neighbour.NeighboorCnt;src_i++)
            {
                VertexID dst = src_neighbour.Neighboor.nextLv1Chunk->NeighboorChunk[src_i].dest;
                auto val = src_neighbour.Neighboor.nextLv1Chunk->NeighboorChunk[src_i].value;
                Weight relax_dist = distance[src] + val;
                if (relax_dist < distance[dst]) {
                    // if(dst == 1048580)
                    // {
                    //     printf("%d %d %d %d %d\n", src, distance[src], val, relax_dist, distance[dst]);
                    // }
                    if (write_min(&distance[dst], relax_dist)) {
                        active_out->set_bit(dst);
                        activated += 1;
                        // record.emplace_back(src, dst);
                        // std::cout << "src: " << src << " dst: "<<dst  \
                        // <<" value: "<< val\
                        // << " ns[src]: " << distance[src] << " oldV: " << old_V<<std::endl; 
                    } 
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
                    auto val = n_ptr.nextLeafChunk->NeighboorChunk[src_i].value;
                    Weight relax_dist = distance[src] + val;
                    if (relax_dist < distance[dst]) {
                        // if(dst == 1048580)
                        // {
                        //     printf("%d %d %d %d %d\n", src, distance[src], val, relax_dist, distance[dst]);
                        // }
                        if (write_min(&distance[dst], relax_dist)) {
                            active_out->set_bit(dst);
                            activated += 1;
                        //     record.emplace_back(src, dst);
                        //     std::cout << "src: " << src << " dst: "<<dst  \
                        // <<" value: "<< val\
                        // << " ns[src]: " << distance[src] << " oldV: " << old_V<<std::endl; 
                        }
                    }
                }
                n_ptr = n_ptr.nextLeafChunk->nextPtr;
            }
        }
        return activated;
      },
      [&](VertexID dst) {
        int flag = 0;
        auto dst_neighbour = graph->cblptr->NodeList[dst];
        if(dst_neighbour.NeighboorCnt <= LV1CHUNK_NCNT)
        {
            for(IndexType dst_i=0;dst_i<dst_neighbour.NeighboorCnt;dst_i++)
            {
                VertexID src = dst_neighbour.Neighboor.nextLv1Chunk->NeighboorChunk[dst_i].dest;
                // if (active_in->get_bit(src)) {
                auto val = dst_neighbour.Neighboor.nextLv1Chunk->NeighboorChunk[dst_i].value;
                Weight relax_dist = distance[src] + val;
                if (relax_dist < distance[dst]) {
                    flag = 1;
                    distance[dst] = relax_dist;
                    active_out->set_bit(dst);
                }
                // }
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
                    // if (active_in->get_bit(src)) {
                    auto val = n_ptr.nextLeafChunk->NeighboorChunk[dst_i].value;
                    Weight relax_dist = distance[src] + val;
                    if (relax_dist < distance[dst]) {
                        flag = 1;
                        distance[dst] = relax_dist;
                        active_out->set_bit(dst);
                    }
                    // }
                }
                n_ptr = n_ptr.nextLeafChunk->nextPtr;
            }
        }
        return flag;
      },
      active_in
    );
    std::swap(active_in, active_out);
    // std::cout<<"iter:"<<i_i<<" = "<<distance[6]<<" actV = "<<active_vertices<<std::endl;
  }
    // std::cout<<"round: "<<i_i<<std::endl;
  auto end = std::chrono::steady_clock::now();
  std::chrono::duration<double, std::micro> elapsed = end - start; // std::micro 表示以微秒为时间单位
  std::cout<< "time: "  << elapsed.count() << "us" << std::endl;
  std::cout<<distance[6]<<std::endl;

  delete active_in;
  delete active_out;
}

int main(int argc, char ** argv) {

  if (argc<5) {
    printf("sssp [file] [infofile] [thread] [root]\n");
    exit(-1);
  }

  Graph<Weight> * graph;
  graph = new Graph<Weight>();
  int threads = std::atoi(argv[3]);
  graph->load_directed(argv[1], argv[2], threads);
  VertexID root = std::atoi(argv[4]);

  compute(graph, root);

  delete graph;
  return 0;
}
