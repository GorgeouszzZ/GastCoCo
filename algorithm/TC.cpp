// #define PRINT_DEBUG_MESSAGES
#include "graph_coro.hpp"
// #include "graph.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <chrono>

#include "../other/atomic.hpp"

#include <math.h>


void compute(Graph<Empty> * graph)
{
    uint64_t count_result = 0;
    VertexID percent = 100;
    VertexID v_cnt = percent;
    int pp = 1;
    auto start = std::chrono::steady_clock::now();
    
    graph->process_edges_all_graph<int,double>(
      [&](VertexID p1_dst) {
        
            // std::cout << p1_dst << "\n";
        
        CoroGraph::CBList::EdgeIterator p1_iter(graph->cblptr, p1_dst);
        while(p1_iter.alive)
        {
            auto p2_src = p1_iter.GetEdegInfo().dest;
            if(p1_dst < p2_src)
            {
                // std::cout << "src:" << p2_src << " "; 
                CoroGraph::CBList::EdgeIterator p1_p3_iter(graph->cblptr, p1_dst);
                CoroGraph::CBList::EdgeIterator p2_p3_iter(graph->cblptr, p2_src);
                while(p1_p3_iter.alive && p2_p3_iter.alive
                    && p1_p3_iter.GetEdegInfo().dest < p1_dst && p2_p3_iter.GetEdegInfo().dest < p2_src)
                {
                    if(p1_p3_iter.GetEdegInfo().dest == p2_p3_iter.GetEdegInfo().dest)
                    {
                        ++p1_p3_iter;
                        ++p2_p3_iter;
                        ++count_result;
                    }
                    else if(p1_p3_iter.GetEdegInfo().dest < p2_p3_iter.GetEdegInfo().dest)
                        ++p1_p3_iter;
                    else
                        ++p2_p3_iter;
                }
            }
            ++p1_iter;
        }
        return 1;
      }
    );
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double, std::micro> elapsed = end - start;
    std::cout << "Triangle Count = " << count_result << std::endl;
    std::cout << "time: " << elapsed.count() << "μs" << std::endl;
}

int main(int argc, char ** argv) {

  if (argc<4) {
    printf("TC [file] [infofile] [thread]\n");
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