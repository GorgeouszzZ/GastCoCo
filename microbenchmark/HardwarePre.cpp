// #include "../other/Coro_graph_v2.hpp"
#include "../CBList/CBList.hpp"
#include <vector>
#include <chrono>
using namespace std;

#define GET_NUM 64
vector<VertexID> record;

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



void GetNeighbor(const CoroGraph::CBList& cbl, VertexID src, int getID)
{
    // print_profile_time("before-start-time: ", getID);

    auto src_neighbour = cbl.NodeList[src];
    auto n_ptr = src_neighbour.Neighboor;
    printf("%p\n", n_ptr.nextLeafChunk);

    // print_profile_time("before-compute-time: ", getID);
    for(IndexType loop_i=0;loop_i<src_neighbour.ChunkCnt;loop_i++)
    {
        // print_profile_time("exec-time1: ", getID);
        profiling_time = std::chrono::steady_clock::now();
        std::cout<<n_ptr.nextLeafChunk->count<<" :";//<<n_ptr.nextLeafChunk<<"  ";
        for(IndexType dst_i=0;dst_i<n_ptr.nextLeafChunk->count;dst_i++)
        {
            VertexID dst = n_ptr.nextLeafChunk->NeighboorChunk[dst_i].dest;
            record.emplace_back(dst);
        }
        n_ptr = n_ptr.nextLeafChunk->nextPtr;
        print_profile_time("exec-time2: ", getID);
    }
    // print_profile_time("end-time: ", getID);
}

int main(int argc, char** argv)
{
    std::string GRAPH_DIR = argv[1];
    std::string GRAPH_INFO_DIR = argv[2];
    auto [V, E] = LoadEVFromInfoFile(GRAPH_INFO_DIR);
    int for_switch = atoi(argv[3]);
    int coro_num = 2;

    CoroGraph::CBList G(V, E, GRAPH_DIR);
    printf("load finished..\n");

    int x,y;//10013,5014
    cin >> x >> y;


    auto start = std::chrono::steady_clock::now();
    if(for_switch == 1)
    {
        GetNeighbor(G, x, 1);
        GetNeighbor(G, y, 2);
    }
    else
    {
        auto src_neighbour = G.NodeList[x];
        auto n_ptr = src_neighbour.Neighboor;
        print_profile_time("before-compute-time: ", x);
        for(IndexType dst_i=0;dst_i<n_ptr.nextLeafChunk->count;dst_i++)
        {
            VertexID dst = n_ptr.nextLeafChunk->NeighboorChunk[dst_i].dest;
            record.emplace_back(dst);
        }
        n_ptr = n_ptr.nextLeafChunk->nextPtr;
        print_profile_time("exec-time: ", x);

        for(IndexType dst_i=0;dst_i<n_ptr.nextLeafChunk->count;dst_i++)
        {
            VertexID dst = n_ptr.nextLeafChunk->NeighboorChunk[dst_i].dest;
            record.emplace_back(dst);
        }
        n_ptr = n_ptr.nextLeafChunk->nextPtr;
        print_profile_time("exec-time: ", x);

        for(IndexType dst_i=0;dst_i<n_ptr.nextLeafChunk->count;dst_i++)
        {
            VertexID dst = n_ptr.nextLeafChunk->NeighboorChunk[dst_i].dest;
            record.emplace_back(dst);
        }
        n_ptr = n_ptr.nextLeafChunk->nextPtr;
        print_profile_time("exec-time: ", x);

        for(IndexType dst_i=0;dst_i<n_ptr.nextLeafChunk->count;dst_i++)
        {
            VertexID dst = n_ptr.nextLeafChunk->NeighboorChunk[dst_i].dest;
            record.emplace_back(dst);
        }
        n_ptr = n_ptr.nextLeafChunk->nextPtr;
        print_profile_time("exec-time: ", x);

        src_neighbour = G.NodeList[y];
        n_ptr = src_neighbour.Neighboor;
        print_profile_time("before-compute-time: ", y);
        for(IndexType dst_i=0;dst_i<n_ptr.nextLeafChunk->count;dst_i++)
        {
            VertexID dst = n_ptr.nextLeafChunk->NeighboorChunk[dst_i].dest;
            record.emplace_back(dst);
        }
        n_ptr = n_ptr.nextLeafChunk->nextPtr;
        print_profile_time("exec-time: ", y);

        for(IndexType dst_i=0;dst_i<n_ptr.nextLeafChunk->count;dst_i++)
        {
            VertexID dst = n_ptr.nextLeafChunk->NeighboorChunk[dst_i].dest;
            record.emplace_back(dst);
        }
        n_ptr = n_ptr.nextLeafChunk->nextPtr;
        print_profile_time("exec-time: ", y);

        for(IndexType dst_i=0;dst_i<n_ptr.nextLeafChunk->count;dst_i++)
        {
            VertexID dst = n_ptr.nextLeafChunk->NeighboorChunk[dst_i].dest;
            record.emplace_back(dst);
        }
        n_ptr = n_ptr.nextLeafChunk->nextPtr;
        print_profile_time("exec-time: ", y);
    }
    auto end = std::chrono::steady_clock::now();

    std::chrono::duration<double, std::micro> elapsed = end - start; 
    std::cout<< "time: "  << elapsed.count() << "us" << std::endl;
    // for(auto& r:record)
    //     std::cout << r << " ";
    // std::cout << std::endl;
    return 0;
}