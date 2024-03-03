#include "../other/graphIO.hpp"
#include "iostream"
#include <chrono>
int main(int argc, char** argv)
{
    if(argc<4)
    {
        std::cout<<"[efile] [edge_num] [isB]\n";
        exit(-1);
    }
    std::string GRAPH_DIR = argv[1];
    int E = atoi(argv[2]);
    int isB = atoi(argv[3]);
    //==============================================================time_start====================
    auto start = std::chrono::steady_clock::now();

    if(isB!=1)
        LoadGraphFromFile(E, GRAPH_DIR);
    else
        LoadGraphFromBGBinaryFile(E, GRAPH_DIR);

    auto end = std::chrono::steady_clock::now();
//==============================================================================time_stop===========================
    std::chrono::duration<double, std::micro> elapsed = end - start; // std::micro 表示以微秒为时间单位
    std::cout<< "time: "  << elapsed.count() << "us" << std::endl;
}