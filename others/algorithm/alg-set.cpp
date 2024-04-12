#include "graph_coro.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <chrono>

#include "../other/atomic.hpp"
#include "../other/prefetch.hpp"
#include "rmat_util.hpp"

#include <math.h>

const double d = (double)0.85;
thread_local tcalloc coroutine_allocator;
double * curr;
double * next;

typedef int Weight;
std::vector<std::pair<int, int>> record;

Weight * distance;
VertexSubset * active_in_sssp;
VertexSubset * active_out_sssp;

#define LABEL 15

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

void Query(Graph<Weight> * graph) {
  auto EL = generate_random_edge_list(graph->cblptr->NodeNum, graph->cblptr->EdgeNum);
  std::vector<bool> EL_result(EL.size());
  omp_set_num_threads(graph->threads);

//   for(int ii=0;ii<20;ii++)
//     std::cout << EL[ii].start_point << " --- " << EL[ii].end_point<< std::endl;

  auto start = std::chrono::steady_clock::now();

  #pragma omp parallel for
  for(IndexType e = 0; e < EL.size(); ++e)
    EL_result[e] = graph->cblptroutgoing->ReadEdge(EL[e].start_point, EL[e].end_point);
  
  auto end = std::chrono::steady_clock::now();
  std::chrono::duration<double, std::micro> elapsed = end - start; // std::micro 表示以微秒为时间单位
  std::cout<< "Query-time: "  << elapsed.count() << "us" << std::endl;
}

void LP(Graph<Weight> * graph, int iterations) {
  auto start = std::chrono::steady_clock::now();

  auto vertices = graph->cblptr->NodeNum;

  auto **lb_cnt = new uint32_t*[LABEL+1];
  for (uint32_t i = 0; i < LABEL+1; ++i) {
    *(lb_cnt + i) = new uint32_t[vertices];
  }

  auto *lb = new uint32_t[vertices];
  #pragma omp parallel for
  for (uint32_t i = 0; i < vertices; ++i) {
    lb[i] = (i%LABEL+LABEL)%LABEL + 1;
  }


  for (IndexType i_i=0;i_i<iterations;i_i++) {

    graph->process_edges_all_graph<int,double>(
      [&](VertexID dst){
        auto dst_neighbour = graph->cblptr->NodeList[dst];
        if(dst_neighbour.NeighboorCnt <= LV1CHUNK_NCNT)
        {
            for(IndexType dst_i=0;dst_i<dst_neighbour.NeighboorCnt;dst_i++)
            {
                VertexID src = dst_neighbour.Neighboor.nextLv1Chunk->NeighboorChunk[dst_i].dest;
                ++lb_cnt[lb[dst]][src];
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
                    ++lb_cnt[lb[dst]][src];
                }
                n_ptr = n_ptr.nextLeafChunk->nextPtr;
            }
        }
        return 1;
      }
    );
    graph->process_vertices_all_graph<double>(
        [&](VertexID vtx) {
            if(vtx == 0) return 1;
            uint32_t max_lb = 0;
            uint32_t max_cnt = 0;
            for(uint32_t ilb = LABEL; ilb>=1 ; --ilb){
                if(max_cnt <= lb_cnt[ilb][vtx]){
                    max_cnt = lb_cnt[ilb][vtx];
                    max_lb = ilb;
                }
            }
            lb[vtx] = max_lb;
            return 1;
        }
    );

    graph->process_vertices_all_graph<double>(
        [&](VertexID vtx) {
            for(uint32_t ilb =1 ; ilb<=LABEL ; ++ilb){
            lb_cnt[ilb][vtx] = 0;
            }
            return 1;
        }
    );
    //or
    // for(uint32_t ilb = 1; ilb<=LABEL; ++ilb)
    //     graph->fill_vertex_array(lb_cnt[ilb], uint32_t(0));
  }

  auto end = std::chrono::steady_clock::now();
  std::chrono::duration<double, std::micro> elapsed = end - start; // std::micro 表示以微秒为时间单位
  std::cout<< "LP-time: "  << elapsed.count() << "us" << std::endl;
  std::cout<<lb[6]<<std::endl;

}
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
                        active_out_sssp->set_bit(dst);
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
                            active_out_sssp->set_bit(dst);
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
                    active_out_sssp->set_bit(dst);
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
                        active_out_sssp->set_bit(dst);
                    }
                }
                n_ptr = n_ptr.nextLeafChunk->nextPtr;
            }
        }
    }
}

void SSSP(Graph<Weight> * graph, VertexID root) {

  auto vertices = graph->cblptr->NodeNum;

  distance = new Weight[vertices];
  active_in_sssp = new VertexSubset(vertices);
  active_out_sssp = new VertexSubset(vertices);
  active_in_sssp->clear();
  active_in_sssp->set_bit(root);
  graph->fill_vertex_array(distance, (Weight)1e9);
  distance[root] = (Weight)0;
  VertexID active_vertices = 1;
  int i_i=0;
  auto start = std::chrono::steady_clock::now();

  for (;active_vertices>0;i_i++)  {

    active_out_sssp->clear();

    active_vertices = graph->coro_process_edges<int,double>(
      coro_sssp_sparse,
      coro_sssp_dense,
      active_in_sssp
    );
    std::swap(active_in_sssp, active_out_sssp);
  }
    std::cout<<"round: "<<i_i<<std::endl;
  auto end = std::chrono::steady_clock::now();
  std::chrono::duration<double, std::micro> elapsed = end - start; // std::micro 表示以微秒为时间单位
  std::cout<< "sssp-time: "  << elapsed.count() << "us" << std::endl;
  std::cout<<distance[6]<<std::endl;

}

void BFS(Graph<Weight> * graph, VertexID root) {

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
  std::cout<< "BFS-time: "  << elapsed.count() << "us" << std::endl;
  

  delete active_in;
  delete active_out;
  delete visited;
}

generator<void> coro_pagerank_process_edge(const CoroGraph::CBList* cbl, VertexID dst_start, VertexID dst_end)
{
  for(VertexID dst = dst_start; dst < dst_end; ++dst)
  {
    auto dst_neighbour = cbl->NodeList[dst];
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

void PageRank(Graph<Weight> * graph, int iterations) {
  auto start = std::chrono::steady_clock::now();

  auto vertices = graph->cblptr->NodeNum;

  curr = new double[vertices];
  next = new double[vertices];
  VertexSubset * active = new VertexSubset(vertices);
  active->fill();

  double delta = graph->process_vertices_all_graph<double>(
    [&](VertexID vtx){
      curr[vtx] = (double)1;
      if (graph->cblptroutgoing->NodeList[vtx].NeighboorCnt>0) {
        curr[vtx] /= graph->cblptroutgoing->NodeList[vtx].NeighboorCnt;
      }
      return (double)1;
    }
  );
  delta /= vertices;


  for (IndexType i_i=0;i_i<iterations;i_i++) {

    graph->fill_vertex_array(next, (double)0);
    graph->coro_process_edges_all_graph<int,double>(
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
          if (graph->cblptroutgoing->NodeList[vtx].NeighboorCnt>0) {
            next[vtx] /= graph->cblptroutgoing->NodeList[vtx].NeighboorCnt;
            return fabs(next[vtx] - curr[vtx]) * graph->cblptroutgoing->NodeList[vtx].NeighboorCnt;
          }
          return fabs(next[vtx] - curr[vtx]);
        }
      );
    }
    delta /= vertices;
    std::swap(curr, next);
    // std::cout << i_i << ": " << curr[6] << std::endl;
  }

  auto end = std::chrono::steady_clock::now();
  std::chrono::duration<double, std::micro> elapsed = end - start; // std::micro 表示以微秒为时间单位
  std::cout<< "PageRank-time: "  << elapsed.count() << "us" << std::endl;
  std::cout<<curr[6]<<std::endl;

  delete active;
}

void CC(Graph<Weight> * graph) {
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
  std::cout<< "CC-time: "  << elapsed.count() << "us" << std::endl;

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

void TC(Graph<Weight> * graph)
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
                        __sync_fetch_and_add(&count_result, 1);
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
    std::cout << "TC-time: " << elapsed.count() << "μs" << std::endl;
}

int main(int argc, char ** argv) {

  if (argc<7) {
    printf("alg-set [file] [infofile] [thread] [coro] [iterations] [root]\n");
    exit(-1);
  }

  Graph<Weight> * graph;
  graph = new Graph<Weight>();
  int threads = std::atoi(argv[3]);
  int coros = std::atoi(argv[4]);
  graph->load_directed(argv[1], argv[2], threads, coros);
  int iterations = std::atoi(argv[5]);
  VertexID root = std::atoi(argv[6]);

  Query(graph);
  BFS(graph, root);
  SSSP(graph, root);
  PageRank(graph, iterations);
  CC(graph);
  LP(graph, iterations);
  TC(graph);
//   for (int run=0;run<5;run++) {
//     compute(graph, iterations);
//   }

  delete graph;
  return 0;
}
