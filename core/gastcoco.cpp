#include "Parameter_input.hpp"
#include "../CBList/CBList.hpp"
#include "../app/copagerank-pull.hpp"
int main(int argc, char ** argv)
{
    #ifdef LOGO
        PrintLOGO();
    #endif
    google::SetVersionString("1.0");
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    
    if(FLAGS_app == "pagerank" || FLAGS_app == "pr")
    {
        if(FLAGS_cm == false)
        {
            GastCoCo::CBList graph(FLAGS_data, GastCoCo::Mixed, FLAGS_o);
            pagerank(graph, FLAGS_t, FLAGS_c, FLAGS_i);
        }
        else
        {
            GastCoCo::CBList graph(FLAGS_data, GastCoCo::Push, FLAGS_o);
            pagerank(graph, FLAGS_t, FLAGS_c, FLAGS_i);
        }
    }
}