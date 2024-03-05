#include <vector>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <iostream>
#include "../other/graphIO.hpp"
#include "omp.h"
#include <algorithm>
#include <random>

// TODO: 去重工作 usa-road 会因为重复边出错

int main(int argc, char ** argv)
{
    if(argc<6)
    {
        std::cout<<"[efile] [ofile] [v_num] [e_num] [uw_to_w?]\n";
        exit(-1);
    }

    std::string GraphDir = argv[1];
    std::string GraphOut = argv[2];
    int V = atoi(argv[3]);
    int E = atoi(argv[4]);
    int uw = atoi(argv[5]);

    std::random_device rd;
    std::mt19937 gen(rd()); 
    std::uniform_int_distribution<> distrib(1, 10000);

    Gedge* EdgeList = newMemory(Gedge, E);

    std::ifstream fin;
    fin.open(GraphDir, std::ios::in);
    std::string line;
    uint32_t MaxV = 0;
    uint32_t ecnt = 0;
    if(fin.is_open()) {
        if(uw != 1){
            while(std::getline(fin, line)) {
                std::istringstream iss(line);
                uint32_t src, dest, val;
                iss >> src >> dest >> val;
                MaxV = std::max(src, MaxV);
                MaxV = std::max(dest, MaxV);
                EdgeList[ecnt] = {src, dest, val};
                ++ecnt;
            }
        }
        else
        {
            while(std::getline(fin, line)) {
                if(line[0] == '#') continue;
                std::istringstream iss(line);
                uint32_t src, dest, val;
                iss >> src >> dest;
                val = distrib(gen);
                MaxV = std::max(src, MaxV);
                MaxV = std::max(dest, MaxV);
                EdgeList[ecnt] = {src, dest, val};
                ++ecnt;
            }
        }
    }
    else
    {
        printf("file load error\n");
    }   
    fin.close();

    printf("load finished.\n");

    std::vector<int> VertexList(MaxV+1, 0);
    omp_set_num_threads(omp_get_max_threads()); 
    #pragma omp parallel for
    for(uint32_t i = 0; i < ecnt; ++i)
    {
        uint32_t s = EdgeList[i].start_point;
        uint32_t d = EdgeList[i].end_point;
        VertexList[s] = 1;
        VertexList[d] = 1;
    }
    
    VertexID vid = 0;
    for(int i = 0; i < MaxV+1; ++i)
    {
        if(VertexList[i] == 1)
        {
            VertexList[i] = vid;
            ++vid;           
        }
    }

    #pragma omp parallel for
    for(uint32_t i = 0; i < ecnt; ++i)
    {
        uint32_t s = EdgeList[i].start_point;
        uint32_t d = EdgeList[i].end_point;
        EdgeList[i].start_point = VertexList[s];
        EdgeList[i].end_point = VertexList[d];
    }
    VertexList.clear();

    std::ofstream outfile (GraphOut, std::ios::out);
    if (!outfile.is_open()) {
      std::cout << "Unable to open outfile: " << GraphOut << std::endl;
      exit(-1);
    }

    for(uint32_t i = 0; i < ecnt; ++i)
    {
        outfile << EdgeList[i].start_point << " " << EdgeList[i].end_point << " " << EdgeList[i].value << "\n";
    }
    outfile.close();
}