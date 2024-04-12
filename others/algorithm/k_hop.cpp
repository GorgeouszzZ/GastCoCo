
// #define PRINT_DEBUG_MESSAGES
#include <stdio.h>
#include <stdlib.h>
#include <chrono>
#include <vector>

#include "graph.hpp"
#include "../other/atomic.hpp"
#include <omp.h>


typedef int Weight;

//TODO: 分开写或者queue是不是会更好？这里选择和GraphStorageExp相同。
void compute(Graph<Weight> * graph, int k) {
    omp_set_num_threads(graph->threads);
  auto start = std::chrono::steady_clock::now();

    uint64_t n = graph->cblptr->NodeNum;
    std::cout<<"imn:"<<n<<std::endl;
    uint32_t nsrc = n/20;
    srand(n);
    #pragma omp parallel for
    for(int i=0;i<nsrc;i++){
        auto src = rand()%n;
        std::vector<VertexID> OneHop = graph->cblptroutgoing->GetNeighbor(src);
        if(k==2)
        {
            for(auto& nei:OneHop)
                graph->cblptroutgoing->GetNeighbor(nei);
        }
    }
 
  auto end = std::chrono::steady_clock::now();
  std::chrono::duration<double, std::micro> elapsed = end - start; // std::micro 表示以微秒为时间单位
  std::cout<< "time: "  << elapsed.count() << "us" << std::endl;
}

int main(int argc, char ** argv) {

  if (argc<4) {
    printf("k_hop [file] [infofile] [k]\n");
    exit(-1);
  }

  Graph<Weight> * graph;
  graph = new Graph<Weight>();
  graph->load_directed(argv[1], argv[2]);
  int k = std::atoi(argv[3]);

  compute(graph, k);

  delete graph;
  return 0;
}
