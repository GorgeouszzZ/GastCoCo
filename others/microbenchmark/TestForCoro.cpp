#include "../other/Coro_graph_v2.hpp"
#include "../CBList/CBList.hpp"
#include <vector>
#include <chrono>
using namespace std;

#define GET_NUM 64
vector<VertexID> record;
thread_local tcalloc coroutine_allocator;

auto profiling_time = std::chrono::steady_clock::now();
auto profiling_end_time = std::chrono::steady_clock::now();
std::chrono::duration<double, std::micro> ptime;

void print_profile_time(std::string text, int coroid)
{
    profiling_end_time = std::chrono::steady_clock::now();
    ptime = profiling_end_time - profiling_time;
    printf("%d %s %fus\n", coroid, text.c_str(), ptime.count());
    profiling_time = std::chrono::steady_clock::now();
}
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

generator<void> CoroGetNeighbor(const CoroGraph::CBList& cbl, VertexID src, int coroid)
{
    print_profile_time("before-start-time: ", coroid);

    auto src_neighbour = cbl.NodeList[src];
    auto n_ptr = src_neighbour.Neighboor;

    print_profile_time("prefetch-before-time: ", coroid);

    prefetch_Chunk(n_ptr.nextLeafChunk);

    print_profile_time("prefetch-after-time: ", coroid);

    co_await suspend_always{};

    print_profile_time("sus-back-time: ", coroid);
    
    for(IndexType loop_i=0;loop_i<src_neighbour.ChunkCnt;loop_i++)
    {
        for(IndexType dst_i=0;dst_i<n_ptr.nextLeafChunk->count;dst_i++)
        {
            VertexID dst = n_ptr.nextLeafChunk->NeighboorChunk[dst_i].dest;
            record.emplace_back(dst);
        }
        n_ptr = n_ptr.nextLeafChunk->nextPtr;

        print_profile_time("exec-time: ", coroid);

        prefetch_Chunk(n_ptr.nextLeafChunk);

        print_profile_time("prefetch-after-time: ", coroid);

        co_await suspend_always{};

        print_profile_time("sus-back-time: ", coroid);
    }
    print_profile_time("end-time: ", coroid);
}

void GetNeighbor(const CoroGraph::CBList& cbl, VertexID src, int getID)
{
    print_profile_time("before-start-time: ", getID);

    auto src_neighbour = cbl.NodeList[src];
    auto n_ptr = src_neighbour.Neighboor;

    print_profile_time("before-compute-time: ", getID);
    for(IndexType loop_i=0;loop_i<src_neighbour.ChunkCnt;loop_i++)
    {
        std::cout<<n_ptr.nextLeafChunk->count<<" :"<<n_ptr.nextLeafChunk<<"  ";
        for(IndexType dst_i=0;dst_i<n_ptr.nextLeafChunk->count;dst_i++)
        {
            VertexID dst = n_ptr.nextLeafChunk->NeighboorChunk[dst_i].dest;
            record.emplace_back(dst);
        }
        n_ptr = n_ptr.nextLeafChunk->nextPtr;
        print_profile_time("exec-time: ", getID);
    }
    print_profile_time("end-time: ", getID);
}

int main(int argc, char** argv)
{
    std::string GRAPH_DIR = argv[1];
    std::string GRAPH_INFO_DIR = argv[2];
    auto [V, E] = LoadEVFromInfoFIle(GRAPH_INFO_DIR);
    int coro_switch = atoi(argv[3]);
    int coro_num = 2;

    CoroGraph::CBList G(V, E, GRAPH_DIR);
    printf("load finished..\n");

    int x,y;//10013,5014
    cin >> x >> y;
    vector<coroutine_handle<>> Tasks(coro_num);
    if(coro_switch == 0)
    {
        Tasks[0]=CoroGetNeighbor(G, x, 1).get_handle();
        Tasks[1]=CoroGetNeighbor(G, y, 2).get_handle();
    }

    int finished = 0;

    auto start = std::chrono::steady_clock::now();

    if(coro_switch == 1)
    {
        GetNeighbor(G, x, 1);
        GetNeighbor(G, y, 2);
    }
    else
    {
        while(finished < Tasks.size())
        {
            for(auto &t : Tasks)
            {
                if(t)
                {
                    if(t.done())
                    {
                        finished++;
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

    std::chrono::duration<double, std::micro> elapsed = end - start; 
    std::cout<< "time: "  << elapsed.count() << "us" << std::endl;
    // for(auto& r:record)
    //     std::cout << r << " ";
    // std::cout << std::endl;
    return 0;
}