#include "../other/Bind_cpu.hpp"
#include "../CBList/CBList.hpp"
#include <iostream>
#include <stdio.h>
#include <chrono>
#include <stack>
#include <queue>
#include "../other/atomic.hpp"
#include "../gemini/rmat_util.hpp"

using namespace std;

double BatchInsertEdge(CoroGraph::CBList& g, vector<Gedge_noWeight>& EdgeList)
{
    uint32_t edgecnt = EdgeList.size();
//==============================================================time_start====================
    auto start = std::chrono::steady_clock::now();

    g.AddEdgeBatch(EdgeList, edgecnt);

    auto end = std::chrono::steady_clock::now();
//==============================================================================time_stop===========================
    std::chrono::duration<double, std::micro> elapsed = end - start; // std::micro 表示以微秒为时间单位
    // std::cout<< "time: " << elapsed.count() << "us" << std::endl;
    return elapsed.count() * 1e-6;
}

int main(int argc, char ** argv)
{
    if(argc<3)
    {
        cout<<"[efile] [infofile] \n";
        exit(-1);
    }
    string GRAPH_DIR = argv[1];
    string GRAPH_INFO_DIR = argv[2];
    auto [V, E] = LoadEVFromInfoFile(GRAPH_INFO_DIR);

    CoroGraph::CBList G(V, E, GRAPH_DIR);
    printf("load graph finish\n");
    std::vector<uint32_t> update_sizes = {10000};
    // std::vector<uint32_t> update_sizes = {10, 100, 1000, 10000, 100000, 1000000, 10000000};//10, 100, 1000 ,10000,,1000000, 10000000
    auto r = random_aspen();
    auto update_times = std::vector<double>();
    size_t n_trials = 1;
    for (IndexType us=0; us<update_sizes.size(); ++us) {
        double avg_insert = 0;
        double avg_delete = 0;
        double avg_read = 0;
        std::cout << "Running batch size: " << update_sizes[us] << std::endl;

        if (update_sizes[us] < 10000000)
            n_trials = 20;
        else n_trials = 5;
        size_t updates_to_run = update_sizes[us];
        for (size_t ts=0; ts<n_trials; ts++) {
            std::vector<Gedge_noWeight> InsertEdgeList(updates_to_run, Gedge_noWeight({0,0}));
            uint32_t num_nodes = V;

            double a = 0.5;
            double b = 0.1;
            double c = 0.1;
            size_t nn = 1 << (log2_up(num_nodes) - 1);
            auto rmat = rMat<uint32_t>(nn, r.ith_rand(100+ts), a, b, c);
            for(uint32_t i = 0; i < updates_to_run; i++) {
                std::pair<uint32_t, uint32_t> edge = rmat(i);
                InsertEdgeList[i].start_point = edge.first;
                InsertEdgeList[i].end_point = edge.second;
            }
            
            auto sortEdge = [&](auto& e1, auto& e2)
            {
                if(e1.start_point != e2.end_point)
                    return e1.start_point < e2.start_point;
                else
                    return e1.end_point < e2.end_point;
            };
            sort(InsertEdgeList.begin(), InsertEdgeList.end(), sortEdge);

            avg_insert += BatchInsertEdge(G, InsertEdgeList);
            //TODO:这个地方不删除的话 和其他对比会有差异。

            // gettimeofday(&t_start, &tzp);
            // for(uint32_t i = 0; i < updates_to_run; i++) {
            //     G->remove_edge(new_srcs[i], new_dests[i]);
            // }
            // gettimeofday(&t_end, &tzp);
            // avg_delete +=  cal_time_elapsed(&t_start, &t_end);
        }
        double time_i = (double) avg_insert / n_trials;
        double insert_throughput = updates_to_run / time_i;
        printf("batch_size = %zu, average insert: %f, throughput %e\n", updates_to_run, time_i, insert_throughput);

        // double time_d = (double) avg_delete / n_trials;
        // double delete_throughput = updates_to_run / time_d;
        // printf("batch_size = %zu, average delete: %f, throughput %e\n", updates_to_run, time_d, delete_throughput);
    }

    

    return EXIT_SUCCESS;
}