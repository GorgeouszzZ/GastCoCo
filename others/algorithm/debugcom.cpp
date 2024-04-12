
#define PRINT_DEBUG_MESSAGES
#include <stdio.h>
#include <stdlib.h>
#include <chrono>

#include "graph.hpp"
#include "../other/atomic.hpp"
#include <omp.h>



int main(int argc, char ** argv) {

  if (argc<4) {
    printf("sssp [file] [infofile] [root]\n");
    exit(-1);
  }

  Graph<Empty> * graph;
  graph = new Graph<Empty>();
  auto [V, E] = LoadEVFromInfoFIle(argv[2]);
  graph->cblptr = new CoroGraph::CBList(V, E, argv[1], true);
  VertexID root = std::atoi(argv[3]);


  delete graph;
  return 0;
}
