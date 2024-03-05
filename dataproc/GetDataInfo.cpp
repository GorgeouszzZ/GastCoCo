#include <vector>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <iostream>
#include "../other/graphIO.hpp"
#include "omp.h"
#include <algorithm>

int main(int argc, char ** argv)
{
    if(argc<2)
    {
        std::cout<<"[efile]\n";
        exit(-1);
    }

    std::vector<uint32_t> VertexList;
    std::vector<Gedge> EdgeList;


    std::string GraphDir = argv[1];

    std::ifstream fin;
    fin.open(GraphDir, std::ios::in);
    std::string line;
    uint32_t MaxV = 0;
    uint32_t ecnt = 0;
    if(fin.is_open()) {
        while(std::getline(fin, line)) {
            if(line[0] == '#') continue;
            std::istringstream iss(line);
            uint32_t src, dest, val;
            iss >> src >> dest;
            MaxV = std::max(src, MaxV);
            MaxV = std::max(dest, MaxV);
            EdgeList.emplace_back(src, dest, val);
            ++ecnt;
        }
    }
    else
    {
        printf("file load error\n");
    }   
    fin.close();

    printf("load finished.\n");

    std::vector<int> VertexDegree(MaxV+1, -1);
    omp_set_num_threads(omp_get_max_threads());
    #pragma omp parallel for
    for(uint32_t i = 0; i < ecnt; ++i)
    {
        uint32_t s = EdgeList[i].start_point;
        uint32_t d = EdgeList[i].end_point;
        if(VertexDegree[s] == -1)
            VertexDegree[s] = 1;
        else
            ++VertexDegree[s];

        if(VertexDegree[d] == -1)
            VertexDegree[d] = 0;
    }
    
    // #pragma omp parallel for
    // for(int v=0;v<VertexDegree.size();++v)
    // {
    //     if(VertexDegree[v] == -1)
    //         std::cout<<v<<std::endl;
    // }

    printf("sorting...\n");
    sort(VertexDegree.begin(), VertexDegree.end());
    printf("sort finished.\n");

    uint32_t invalid_V = 0;
    for(;invalid_V<MaxV+1;++invalid_V)
    {
        if(VertexDegree[invalid_V] != -1)
            break;
    }

    std::cout<<"MaxV: "<<MaxV<<"\niV: "<<invalid_V<<std::endl;
    uint32_t V = MaxV + 1 - invalid_V;
    double avg_degree = ((double)ecnt)/((double)V);
    uint32_t median = V/2  + invalid_V;
    int m_degree = VertexDegree[median];

    std::cout << "V: " << V << "\nE: " << ecnt << "\navgD: " << avg_degree << "\nmD: " << m_degree << std::endl;
    std::cout << "MaxD: " << VertexDegree[MaxV] << "\nMinD: " << VertexDegree[invalid_V] << std::endl;
    if(V%2 == 1) std::cout << "mD2: " << VertexDegree[median-1] << std::endl;
}