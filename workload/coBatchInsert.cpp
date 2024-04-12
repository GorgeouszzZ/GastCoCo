#include "../other/Bind_cpu.hpp"
#include "../CBList/CBList.hpp"
#include <iostream>
#include <stdio.h>
#include <chrono>
#include <stack>
#include <queue>
#include "../other/rmat_util.hpp"
#include "../other/Slice_for_CBL_coro.hpp"
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <stdio.h>
#include <coroutine>
#include <stack>
#include "../other/Coro_graph_v2.hpp"
#include "../other/atomic.hpp"

using namespace std;

thread_local tcalloc coroutine_allocator;


#define GET_NUM 64

inline void prefetch(const void* ptr) 
{
    typedef struct { char x[GET_NUM]; } cacheline_t;
    asm volatile("prefetcht0 %0" : : "m"(*(const cacheline_t*)ptr));
  //__builtin_prefetch(*(const cacheline_t*)ptr));
}

void prefetch_Chunk(const void* ptr)
{
    for (int i = 0; i < 4 * 64; i += 64)
      prefetch((const char*)ptr + i);
}


generator<void> CoInsertGraph(const CoroGraph::CBList& cbl, const vector<Gedge_noWeight>& EdgeList, IndexType insert_start, IndexType insert_end)
{
    // co_await std::suspend_always{};
    IndexType processing_index = insert_start;
    while(processing_index < insert_end)
    {
        // if(processing_index >= insert_end)
        // {
        //     cout << "gogogo!@\n";
        //     break;
        // }
        // cout << "im " << insert_start << "--" << insert_end << "    " << processing_index << endl;
        auto InsertINFO = EdgeList[processing_index];
        ++processing_index;
        AdjUnit InsertINFO_adj = { InsertINFO.end_point, 0 };
        int flag = 0;
        if(cbl.NodeList[InsertINFO.start_point].NeighboorCnt < LV1CHUNK_NCNT)
        {
            cbl.NodeList[InsertINFO.start_point].Neighboor.nextLv1Chunk->NeighboorChunk[cbl.NodeList[InsertINFO.start_point].Neighboor.nextLv1Chunk->count] = InsertINFO_adj;
            ++cbl.NodeList[InsertINFO.start_point].Neighboor.nextLv1Chunk->count;
            ++cbl.NodeList[InsertINFO.start_point].NeighboorCnt;
            flag = 1;
        }
        else if(cbl.NodeList[InsertINFO.start_point].NeighboorCnt == LV1CHUNK_NCNT)
        {
            ++cbl.NodeList[InsertINFO.start_point].Level;
            for(IndexType i=0;i<cbl.NodeList[InsertINFO.start_point].NeighboorCnt;++i)
            {
                cbl.NodeList[InsertINFO.start_point].BPT.Insert(cbl.NodeList[InsertINFO.start_point].Neighboor.nextLv1Chunk->NeighboorChunk[i]);
            }
            cbl.NodeList[InsertINFO.start_point].BPT.Insert({InsertINFO.end_point, 0});
            ++cbl.NodeList[InsertINFO.start_point].NeighboorCnt;
            auto LastLeaf = cbl.NodeList[InsertINFO.start_point].BPT.GetLeafEnd();
            LastLeaf->nextType = cbl.NodeList[InsertINFO.start_point].Neighboor.nextLv1Chunk->nextType;
            LastLeaf->nextPtr = cbl.NodeList[InsertINFO.start_point].Neighboor.nextLv1Chunk->nextPtr;
            // delete cbl.NodeList[InsertINFO.start_point].Neighboor.nextLv1Chunk;
            cbl.NodeList[InsertINFO.start_point].Neighboor.nextLeafChunk = cbl.NodeList[InsertINFO.start_point].BPT.GetLeafStart();
            cbl.NodeList[InsertINFO.start_point].ChunkCnt = cbl.NodeList[InsertINFO.start_point].BPT.LeafCnt;
            flag = 2;
        }
        else
        {
            cbl.NodeList[InsertINFO.start_point].NeighboorCnt++;
            IndexType kk_forfind = 0;
            CoroGraph::Node* p = cbl.NodeList[InsertINFO.start_point].BPT.Root;
            CoroGraph::InterNode* q;
            while (p != nullptr)
            {
                // prefetch_Chunk(p);
                co_await std::suspend_always{};
                if (p->IsLeaf)
                    break;
                for (kk_forfind = 0; kk_forfind < p->count; kk_forfind++)
                {
                    if (InsertINFO.end_point < ((CoroGraph::InterNode*)p)->KeyList[kk_forfind])
                        break;
                }
                q = (CoroGraph::InterNode*)p;
                p = q->Child[kk_forfind];
            }
            CoroGraph::LeafChunk* InsertPos = (CoroGraph::LeafChunk*)p;
            cbl.NodeList[InsertINFO.start_point].BPT.InsertKnowPos(InsertINFO_adj, InsertPos);
            cbl.NodeList[InsertINFO.start_point].ChunkCnt = cbl.NodeList[InsertINFO.start_point].BPT.LeafCnt;
            flag = 1;
        }
        if(flag == 0){ printf("error"); exit(-1); }
        int src = InsertINFO.start_point;
        if(flag == 2 && src != 0)
        {
            int level = cbl.NodeList[src-1].Level;
            if(level == 1)
            {
                cbl.NodeList[src-1].Neighboor.nextLv1Chunk->nextPtr = cbl.NodeList[src].Neighboor;
                cbl.NodeList[src-1].Neighboor.nextLv1Chunk->nextType = LEAFCHUNK_LEVEL;
            }
            else if(level == 2)
            {
                cbl.NodeList[src-1].BPT.GetLeafEnd()->nextPtr = cbl.NodeList[src].Neighboor;
                cbl.NodeList[src-1].BPT.GetLeafEnd()->nextType = LEAFCHUNK_LEVEL;
            }
        }

        // co_await std::suspend_always{};
    }
}


int thread_num = 16;
int coro_num = 6;

double BatchInsertEdge(CoroGraph::CBList& g, vector<Gedge_noWeight>& EdgeList)
{
    uint32_t edgecnt = EdgeList.size();
    int all_coro_num = coro_num * thread_num;
    if(all_coro_num > EdgeList.size())
    {
        printf("TODO: edgelist insert < coro_num.\n");
        return 0.0f;
    }
    std::vector<uint32_t> parts;
    std::vector<int> parts_index_for_thread(all_coro_num + 1, 0);
//==============================================================time_start====================
    auto start = std::chrono::steady_clock::now();

    // generate partitions array

    VertexID cur_src = EdgeList[0].start_point;
    parts.emplace_back(0);
    for (IndexType i = 1; i < EdgeList.size(); i++) {
        if (cur_src != EdgeList[i].start_point) {
            parts.emplace_back(i);
            cur_src = EdgeList[i].start_point;
        }
    }
    parts.emplace_back(EdgeList.size());

    if(all_coro_num > parts.size() - 1)
    {
        printf("TODO: src insert num < coro_num.\n");
        return 0.0f;
    }

    int average_part_per_coro = (parts.size() - 1) / all_coro_num;
    int remain_part = (parts.size() - 1) % all_coro_num;
    for(IndexType i = 1; i < all_coro_num; ++i)
    {
        parts_index_for_thread[i] = parts_index_for_thread[i-1] + average_part_per_coro;
        if(i <= remain_part)
            ++parts_index_for_thread[i];     
    }
    parts_index_for_thread[all_coro_num] = parts.size()-1;

    omp_set_num_threads(thread_num);
    #pragma omp parallel for
    for(IndexType tid = 0; tid < thread_num; ++tid)
    { 
        std::vector<coroutine_handle<>> InsertTasks(coro_num);
        for(uint32_t i = 0; i<coro_num; ++i)
        {   
            // cout << parts[parts_index_for_thread[tid * coro_num + i]] << "-" << parts[parts_index_for_thread[tid * coro_num + i + 1]] << "\n";
            InsertTasks[i]=CoInsertGraph(g, EdgeList, parts[parts_index_for_thread[tid * coro_num + i]], parts[parts_index_for_thread[tid * coro_num + i + 1]]).get_handle();
        }

        int finished = 0;
        while(finished < InsertTasks.size())
        {
            for(auto &t : InsertTasks)
            {
                if(t)
                {
                    if(t.done())
                    {
                        ++finished;
                        t.destroy();
                        t = nullptr;
                    }
                    else
                    {
                        t.resume();
                    }
                }
            }
        }
    }
    
    auto end = std::chrono::steady_clock::now();
//==============================================================================time_stop===========================
    std::chrono::duration<double, std::micro> elapsed = end - start; // std::micro 表示以微秒为时间单位
    // std::cout<< "time: " << elapsed.count() << "us" << std::endl;
    return elapsed.count() * 1e-6;
}

int main(int argc, char ** argv)
{
    if(argc<5)
    {
        cout<<"[efile] [infofile] [thread] [coro]\n";
        exit(-1);
    }
    string GRAPH_DIR = argv[1];
    string GRAPH_INFO_DIR = argv[2];
    auto [V, E] = LoadEVFromInfoFile(GRAPH_INFO_DIR);
    thread_num = atoi(argv[3]);
    coro_num = atoi(argv[4]);
    auto upsz = atoi(argv[5]);

    CoroGraph::CBList G(V, E, GRAPH_DIR);
    printf("load graph finish\n");
    std::vector<uint32_t> update_sizes;
    update_sizes.emplace_back(upsz);
    // std::vector<uint32_t> update_sizes = {10000};
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