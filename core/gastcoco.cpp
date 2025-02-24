#include "parameter_input.hpp"
#include "CBList.hpp"
#include "copagerank_pull.hpp"

int main(int argc, char **argv)
{
#ifdef LOGO
    PrintLOGO();
#endif
    google::SetVersionString("1.0");
    google::ParseCommandLineFlags(&argc, &argv, true);

    if (FLAGS_app == "pagerank" || FLAGS_app == "pr")
    {
        if (FLAGS_cm == false)
        {
            // GastCoCo::CBList graph(FLAGS_data, GastCoCo::ComputeMode::Mixed, FLAGS_o);
            // pagerank(graph, FLAGS_t, FLAGS_c, FLAGS_i);
        }
        else
        {
            // use pull only
            GastCoCo::CBList graph(FLAGS_data, GastCoCo::ComputeMode::Pull, FLAGS_o);
            pagerank(graph, FLAGS_t, FLAGS_c, FLAGS_i);
        }
    }
}