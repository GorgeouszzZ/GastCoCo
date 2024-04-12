#include <iostream>
#include <fstream>
#include <string>
#include <gflags/gflags.h>
#include <vector>
#include "../other/type.hpp"
#include "omp.h"
#include "../other/graphIO.hpp"
#include "../workload/Slice_for_CBL_coro.hpp"

DEFINE_bool(sym, true, "Symmetric");
DEFINE_string(iFile, "", "Transfer input file");
DEFINE_string(oFile, "", "Transfer input file");


long get_memory_usage()
{
    FILE*file=fopen("/proc/meminfo","r");
    if(file == NULL){
        fprintf(stderr,"cannot open /proc/meminfo\n");
        return -1;
    }
    char keyword[20];
    char valuech[20];
    double mem        =0;
    double free_mem   =0;
    fscanf(file,"MemTotal: %s kB\n",keyword);
    mem=double(atol(keyword))/(1024*1024);
    fscanf(file,"MemFree: %s kB\n",valuech);
    fscanf(file,"MemAvailable: %s kB\n",valuech);
    free_mem=double(atol(valuech))/(1024*1024);
    fclose(file);
    fprintf(stderr,"Memory %.3f GB \\ %.3f GB.\n", mem, free_mem);
    return (long)(free_mem);
}

// void transfer_twitter(){

//     std::ifstream infile;
//     infile.open(FLAGS_iFile, std::ios::in);
//     if (!infile.is_open()){
//         std::cout << "Open File Failed." << std::endl;
//     }
//     std::string s;
//     getline(infile,s);
//     EdgeID NumEdge;
//     VertexID MaxSrc, MaxDst;

//     std::cout << s << std::endl;

//     std::vector<Gedge_noWeight> Ev;

//     VertexID src = 0,dst = 0;
//     NumEdge = 0;
//     while(infile >> src){
//         infile >> dst;
//         Ev.emplace_back(Gedge_noWeight(src , dst));
//         MaxSrc = std::max(MaxSrc,src);
//         MaxDst = std::max(MaxDst,dst);
//         ++NumEdge;
//     }
//     std::cout << src<< ' ' << dst << std::endl;
//     std::cout << MaxSrc << ' ' << MaxDst << ' ' << NumEdge << ' ' << Ev.size() << std::endl;
//     std::cout << "memory " << get_memory_usage() << std::endl;

//     auto *E = newMemory(Gedge_noWeight, NumEdge);
//     for(EdgeID i = 0;i<Ev.size();++i){
//         E[i] = Ev[i];
//     }
//     Ev.clear();
//     std::cout << "memory " << get_memory_usage() << std::endl;

//     std::ofstream outfile (FLAGS_oFile, std::ios::out | std::ios::binary);
//     if (!outfile.is_open()) {
//       std::cout << "Unable to open file: " << FLAGS_oFile << std::endl;
//       exit(-1);
//     }
    
//     outfile.write(E, NumEdge);

//     outfile.close();
//     free(E);
// }
void transfer_normal(){

    std::ifstream infile;
    infile.open(FLAGS_iFile, std::ios::in);
    if (!infile.is_open()){
        std::cout << "Open File Failed." << std::endl;
    }
    EdgeID NumEdge = 0;
    VertexID MaxSrc = 0;
    VertexID MaxDst = 0;


    std::vector<Gedge_noWeight> Ev;

    VertexID src = 0,dst = 0;
    NumEdge = 0;
    while(infile >> src){
        infile >> dst;
        Ev.emplace_back(Gedge_noWeight(src , dst));
        MaxSrc = std::max(MaxSrc,src);
        MaxDst = std::max(MaxDst,dst);
        ++NumEdge;
    }
    std::cout << src<< ' ' << dst << std::endl;
    std::cout << MaxSrc << ' ' << MaxDst << ' ' << NumEdge << ' ' << Ev.size() << std::endl;
    std::cout << "memory " << get_memory_usage() << std::endl;

    long n = Ev.size() * 2;
    int* Elist = new int[n];
    omp_set_num_threads(omp_get_max_threads());
    #pragma omp parallel for
    for(int i=0;i<Ev.size();++i)
    {
        Elist[2 * i] = Ev[i].start_point;
        Elist[2 * i + 1] = Ev[i].end_point;
    }
    Ev.clear();
    
    std::cout << "memory " << get_memory_usage() << std::endl;


    std::ofstream outfile (FLAGS_oFile, std::ios::out | std::ios::binary);
    if (!outfile.is_open()) {
      std::cout << "Unable to open outfile: " << FLAGS_oFile << std::endl;
      exit(-1);
    }
    
    long BSIZE = 1000000;
    long offset = 0;
    n *= sizeof(int);
    while (offset < n) {
      // Generates a string for a sequence of size at most BSIZE
      // and then wrties it to the output stream
      outfile.write((const char *)(Elist+offset/4), std::min(BSIZE,n-offset));
      offset += BSIZE;
    } 

    outfile.close();
    free(Elist);
}

void transfer_normal_weight(){

    std::ifstream infile;
    infile.open(FLAGS_iFile, std::ios::in);
    if (!infile.is_open()){
        std::cout << "Open File Failed." << std::endl;
    }
    EdgeID NumEdge = 0;
    VertexID MaxSrc = 0;
    VertexID MaxDst = 0;


    std::vector<Gedge> Ev;

    VertexID src = 0,dst = 0;
    int val = 0;
    NumEdge = 0;
    while(infile >> src){
        infile >> dst >> val;
        Ev.emplace_back(Gedge(src, dst, val));
        MaxSrc = std::max(MaxSrc,src);
        MaxDst = std::max(MaxDst,dst);
        ++NumEdge;
    }

    std::cout << "memory " << get_memory_usage() << std::endl;

    long n = Ev.size() * 3;
    int* Elist = new int[n];
    omp_set_num_threads(omp_get_max_threads());
    #pragma omp parallel for
    for(int i=0;i<Ev.size();++i)
    {
        Elist[3 * i] = Ev[i].start_point;
        Elist[3 * i + 1] = Ev[i].end_point;
        Elist[3 * i + 2] = Ev[i].value;
    }
    Ev.clear();
    
    std::cout << "memory " << get_memory_usage() << std::endl;


    std::ofstream outfile (FLAGS_oFile, std::ios::out | std::ios::binary);
    if (!outfile.is_open()) {
      std::cout << "Unable to open outfile: " << FLAGS_oFile << std::endl;
      exit(-1);
    }
    
    long BSIZE = 1000000;
    long offset = 0;
    n *= sizeof(int);
    while (offset < n) {
      // Generates a string for a sequence of size at most BSIZE
      // and then wrties it to the output stream
      outfile.write((const char *)(Elist+offset/4), std::min(BSIZE,n-offset));
      offset += BSIZE;
    } 

    outfile.close();
    free(Elist);
}

void transfer_normal_BG_weight(){
    std::ifstream infile;
    infile.open(FLAGS_iFile, std::ios::in);
    if (!infile.is_open()){
        std::cout << "Open File Failed." << std::endl;
    }
    EdgeID NumEdge = 0;
    VertexID MaxSrc = 0;
    VertexID MaxDst = 0;


    std::vector<Gedge> Ev;

    VertexID src = 0,dst = 0;
    int val = 0;
    NumEdge = 0;
    while(infile >> src){
        infile >> dst >> val;
        Ev.emplace_back(Gedge(src, dst, val));
        MaxSrc = std::max(MaxSrc,src);
        MaxDst = std::max(MaxDst,dst);
        ++NumEdge;
    }

    std::cout << "memory " << get_memory_usage() << std::endl;


    auto sliceResult = SliceTaskForCoro(Ev.size(), 2000);
    std::ofstream outfile (FLAGS_oFile, std::ios::out | std::ios::binary);
    if (!outfile.is_open()) {
      std::cout << "Unable to open outfile: " << FLAGS_oFile << std::endl;
      exit(-1);
    }
    for(int slice=1; slice<sliceResult.size(); ++slice)
    {
        int n = sliceResult[slice] - sliceResult[slice - 1];
        int N = n * 3;
        int* Elist = new int[N];
        int start_Vid = sliceResult[slice - 1];
        omp_set_num_threads(omp_get_max_threads());
        #pragma omp parallel for
        for(int i=0;i<n;++i)
        {
            Elist[3 * i] = Ev[i + start_Vid].start_point;
            Elist[3 * i + 1] = Ev[i + start_Vid].end_point;
            Elist[3 * i + 2] = Ev[i + start_Vid].value;
        }
        // std::cout << "memory " << get_memory_usage() << std::endl;

            
        long BSIZE = 120000;
        long offset = 0;
        N *= sizeof(int);
        while (offset < N) {
        // Generates a string for a sequence of size at most BSIZE
        // and then wrties it to the output stream
            outfile.write((const char *)(Elist+offset/4), std::min(BSIZE, N-offset));
            offset += BSIZE;
        } 
        free(Elist);
    }
    outfile.close();
}

void transfer_tsv(){
    std::ifstream infile;
    infile.open(FLAGS_iFile, std::ios::in);
    if (!infile.is_open()){
        std::cout << "Open File Failed." << std::endl;
    }
    EdgeID NumEdge = 0;
    VertexID MaxSrc = 0;
    VertexID MaxDst = 0;
    std::string s,t;
    getline(infile,s);
    infile >> t >> NumEdge >> MaxSrc >> MaxDst;

    std::cout << s << std::endl << t << ' ' << NumEdge << ' ' << MaxSrc << ' ' << MaxDst << std::endl;

    uint32_t* E = newMemory(uint32_t, NumEdge * 2);
    uint32_t src,dst;
    for (int i = 0; i < NumEdge ; ++i){
        infile >> src >> dst;
        E[2 * i] = src;
        E[2 * i + 1] = dst; 
    }

    std::ofstream outfile (FLAGS_oFile, std::ios::out | std::ios::binary);
    if (!outfile.is_open()) {
      std::cout << "Unable to open outfile: " << FLAGS_oFile << std::endl;
      exit(-1);
    }
    
    long BSIZE = 1200000;
    long offset = 0;
    NumEdge *= sizeof(int)*2;
    while (offset < NumEdge) {
      // Generates a string for a sequence of size at most BSIZE
      // and then wrties it to the output stream
      outfile.write((const char *)(E+offset/4), std::min(BSIZE,NumEdge-offset));
      offset += BSIZE;
    } 

    outfile.close();
    free(E);
}


void Readtest01()
{
	std::ifstream ifs;
	ifs.open(FLAGS_oFile, std::ios::in | std::ios::binary);
	if (!ifs.is_open())	// 打开失败返回0
	{
	    std::cout << "文件打开失败" << std::endl;
		return;
	}
	int* p2 = new int[120];
	ifs.read( (char *)p2, 120 * sizeof(int));
    for(int i=0;i<40;++i)
        std::cout<<p2[3*i]<<"-"<<p2[3*i+1]<<"-"<<p2[3*i+2]<<std::endl;
	ifs.close();
}


int main(int argc, char** argv)
{
    ::gflags::ParseCommandLineFlags(&argc, &argv, true);
    transfer_normal_BG_weight();
    // Readtest01();
}