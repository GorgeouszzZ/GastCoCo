#pragma once
#include "type.hpp"
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include "omp.h"
#include "../other/SliceTask.hpp"

#define newMemory(__E,__n) (__E*) malloc((__n)*sizeof(__E))
std::vector<Gedge> LoadGraphFromFile(EdgeID E, std::string GRAPH_DIR)
{
    std::vector<Gedge> EdgeList(E);
    std::cout << "file_path=" << GRAPH_DIR << std::endl;

    std::ifstream fin;
    fin.open(GRAPH_DIR, std::ios::in);
    std::string line;
    if(fin.is_open()) {
        EdgeID e = 0;
        while(std::getline(fin, line)) {
            if(line[0] == '#') continue;
            std::istringstream iss(line);
            uint32_t src, dest, val;
            iss>>src>>dest>>val;
            EdgeList[e] = {src, dest, val};
            ++e;
        }
    }
    else
    {
        printf("file load error\n");
    }   
    fin.close();
    // std::cout<<EdgeList.size()<<std::endl;
    return EdgeList;
}

std::vector<Gedge> LoadGraphFromFileNoWeight(EdgeID E, std::string GRAPH_DIR)
{
    std::vector<Gedge> EdgeList(E);
    std::cout << "file_path=" << GRAPH_DIR << std::endl;

    std::ifstream fin;
    fin.open(GRAPH_DIR, std::ios::in);
    std::string line;
    if(fin.is_open()) {
        EdgeID e = 0;
        while(std::getline(fin, line)) {
            if(line[0] == '#') continue;
            std::istringstream iss(line);
            uint32_t src, dest;
            iss>>src>>dest;
            EdgeList[e] = {src, dest, random(10000)};
            ++e;
        }
    }
    else
    {
        printf("file load error\n");
    }   
    fin.close();
    // std::cout<<EdgeList.size()<<std::endl;
    return EdgeList;
}


std::vector<Gedge> LoadGraphFromFilePart(std::string GRAPH_DIR,  uint64_t startline, uint64_t endline)
{
    std::vector<Gedge> EdgeList;
    std::ifstream in(GRAPH_DIR);
    EdgeID edge_cnts=0;
    std::string line;
    if(in) {
        while (std::getline(in,line) && edge_cnts<startline)
        {
            if(line[0] == '#') continue;
            ++edge_cnts;
        }
        while (std::getline(in,line) && edge_cnts<endline)
        {
            if(line[0] == '#') continue;
            std::istringstream iss(line);
            uint32_t src, dest;
            iss>>src>>dest;
            EdgeList.emplace_back(src, dest, random(10000));
        }
    }   
    in.close();
    return EdgeList;
}

std::vector<Gedge> LoadGraphFromBinaryFile(EdgeID E, std::string GRAPH_DIR) {
    std::ifstream file (GRAPH_DIR, std::ios::in | std::ios::binary);
    if(!file.is_open())
    {
        std::cout << "Unable to open file: " << GRAPH_DIR << std::endl;
        abort();
    }
    int* Elist = new int[E*3];
    file.read((char*)Elist, 3 * E * sizeof(int));
    file.close();
    std::vector<Gedge> EdgeList(E);
    omp_set_num_threads(omp_get_max_threads());
    #pragma omp parallel for
    for(int i=0;i<E;++i)
    {
        EdgeList[i].start_point = Elist[3 * i];
        EdgeList[i].end_point  = Elist[3 * i + 1];
        EdgeList[i].value  = Elist[3 * i + 2];
    }
    return EdgeList;
}

std::vector<Gedge> LoadGraphFromBinaryFileNoWeight(EdgeID E, std::string GRAPH_DIR) {
    std::ifstream file (GRAPH_DIR, std::ios::in | std::ios::binary);
    if(!file.is_open())
    {
        std::cout << "Unable to open file: " << GRAPH_DIR << std::endl;
        abort();
    }
    int* Elist = new int[E*2];
    file.read((char*)Elist, 2 * E * sizeof(int));
    file.close();
    std::vector<Gedge> EdgeList(E);
    omp_set_num_threads(omp_get_max_threads());
    #pragma omp parallel for
    for(int i=0;i<E;++i)
    {
        EdgeList[i].start_point = Elist[2 * i];
        EdgeList[i].end_point  = Elist[2 * i + 1];
        EdgeList[i].value  = random(10000);
    }
    return EdgeList;
}

std::vector<Gedge> LoadGraphFromBGBinaryFile(EdgeID E, std::string GRAPH_DIR) {
    std::ifstream file (GRAPH_DIR, std::ios::in | std::ios::binary);
    if(!file.is_open())
    {
        std::cout << "Unable to open file: " << GRAPH_DIR << std::endl;
        abort();
    }
    auto sliceResult = SliceTaskForN(E, 3);
    std::vector<Gedge> EdgeList(E);
    for(int i_i=1; i_i<sliceResult.size(); ++i_i)
    {
        int n = sliceResult[i_i] - sliceResult[i_i-1];
        int* Elist = new int[n*3];
        file.read((char*)Elist, 3 * n * sizeof(int));
        int start_Vid = sliceResult[i_i-1];

        omp_set_num_threads(omp_get_max_threads());
        #pragma omp parallel for
        for(int i=0;i<n;++i)
        {
            EdgeList[i + start_Vid].start_point = Elist[3 * i];
            EdgeList[i + start_Vid].end_point  = Elist[3 * i + 1];
            EdgeList[i + start_Vid].value  = Elist[3 * i + 2];
            // if(i < 10)
            // std::cout << Elist[3*i] << " " << Elist[3*i+1] << " " << Elist[3*i+2] << "|||" << EdgeList[i+start_Vid].start_point << EdgeList[i+start_Vid].end_point << std::endl;
        }
    }

    file.close();
    return EdgeList;
}

std::pair<VertexID, EdgeID> LoadEVFromInfoFile(std::string GRAPH_DIR)
{
    std::ifstream fin;
    fin.open(GRAPH_DIR, std::ios::in);
    std::string line;
    VertexID V;
    EdgeID E;
    if(fin.is_open()) {
            std::getline(fin, line);
            std::istringstream iss(line);
            iss>>V>>E;
    }
    else
    {
        printf("infofile load error\n");
    }   
    fin.close();
    return std::make_pair(V, E);
}