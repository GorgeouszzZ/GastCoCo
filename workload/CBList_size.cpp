#include "../Bind_cpu.hpp"
#include "CBList.hpp"
#include "../Initalize_container.hpp"
#include <iostream>
#include <stdio.h>
#include <chrono>
#include <stack>
#include <queue>

using namespace std;


int main(int argc, char ** argv)
{
    if(argc<4)
    {
        cout<<"[efile] [edge_num] [node_num]\n";
        exit(-1);
    }
    string GRAPH_DIR = argv[1];
    int E = atoi(argv[2]);
    int V = atoi(argv[3]);
    BindCpu(2);
    CBList test1(V, E, GRAPH_DIR);
    unsigned long int CBL_size = sizeof(test1);
    for(int i=0;i<V;i++)
    {
        if(test1.NodeList[i].Level==1)
            CBL_size += 64;
        else if(test1.NodeList[i].Level==2)
        {
            queue<Node*> TreeNode;
            TreeNode.emplace(test1.NodeList[i].BPT.Root);
            while(!TreeNode.empty())
            {
                Node* tmp = TreeNode.front();
                TreeNode.pop();
                CBL_size += 64*4;
                if(tmp->IsLeaf == false)
                {
                    for(int kk=0;kk<=tmp->count;kk++)   
                        TreeNode.emplace(((InterNode*)tmp)->Child[kk]);
                }
            }
        }
    }
    printf("size = %ld\n",CBL_size);
    return EXIT_SUCCESS;
}