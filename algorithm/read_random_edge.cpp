
// #define PRINT_DEBUG_MESSAGES
#include <stdio.h>
#include <stdlib.h>
#include <chrono>

#include "graph.hpp"
#include "rmat_util.hpp"
#include "../other/atomic.hpp"
#include <omp.h>


typedef int Weight;

std::vector<Gedge_noWeight> generate_random_edge_list(VertexID max_Vid, EdgeID edges_num)
{
    uint32_t updates = edges_num/20;
    std::vector<Gedge_noWeight> edge_list(updates,{0,0});
    auto r = random_aspen();
    uint64_t n = max_Vid;
    double a = 0.5;
    double b = 0.1;
    double c = 0.1;
    size_t nn = 1 << (log2_up(n) - 1);
    auto rmat = rMat<uint32_t>(nn, r.ith_rand(100), a, b, c);
    for( uint32_t i = 0; i < updates; i++) {
        std::pair<uint32_t, uint32_t> edge = rmat(i);
        edge_list[i].start_point = edge.first;
        edge_list[i].end_point = edge.second;
    }
    std::cout<<"n = "<<max_Vid << " nn = " << nn <<" update = " << updates<<std::endl;
    return edge_list;
}

void compute(Graph<Weight> * graph) {
  auto EL = generate_random_edge_list(graph->cblptr->NodeNum, graph->cblptr->EdgeNum);
  std::vector<bool> EL_result(EL.size());
  omp_set_num_threads(graph->threads);

  for(int ii=0;ii<20;ii++)
    std::cout << EL[ii].start_point << " --- " << EL[ii].end_point<< std::endl;

  auto start = std::chrono::steady_clock::now();

  #pragma omp parallel for
  for(IndexType e = 0; e < EL.size(); ++e)
    EL_result[e] = graph->cblptroutgoing->ReadEdge(EL[e].start_point, EL[e].end_point);
  
  auto end = std::chrono::steady_clock::now();
  std::chrono::duration<double, std::micro> elapsed = end - start; // std::micro 表示以微秒为时间单位
  std::cout<< "time: "  << elapsed.count() << "us" << std::endl;
}

int main(int argc, char ** argv) {

  if (argc<4) {
    printf("read_random_edge [file] [infofile] [thread]\n");
    exit(-1);
  }

  Graph<Weight> * graph;
  graph = new Graph<Weight>();
  graph->load_directed(argv[1], argv[2], std::atoi(argv[3]));

  compute(graph);

  delete graph;
  return 0;
}
