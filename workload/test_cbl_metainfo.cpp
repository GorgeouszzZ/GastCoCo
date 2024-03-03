#include "../CBList/CBList.hpp"

using namespace std;

int main(int argc, char ** argv)
{
    string GRAPH_DIR = argv[1];
    string GRAPH_INFO_DIR = argv[2];
    auto [V, E] = LoadEVFromInfoFile(GRAPH_INFO_DIR);

    CoroGraph::CBList G(V,E,GRAPH_DIR);
    printf("load graph finish\n");
    CoroGraph::MetaInfo mt(G);
    mt.getInfo();
    
    return EXIT_SUCCESS;
}