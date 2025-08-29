#include "parameter_input.hpp"
#include "CBList.hpp"
#include "copagerank_pull.hpp"

GastCoCo::CBList* BuildGraph(std::string data_info_path, GastCoCo::ComputeMode compute_mode, bool origin_order)
{
#ifdef DEBUG
    if (compute_mode == GastCoCo::ComputeMode::Pull)
        std::cout << "Building [PULL] graph from [" << data_info_path << "] by origin order[" << origin_order << "]." << std::endl;
    else if (compute_mode == GastCoCo::ComputeMode::Push)
        std::cout << "Building [PUSH] graph from [" << data_info_path << "] by origin order[" << origin_order << "]." << std::endl;
    else
        std::cout << "Building [MIXED] graph from [" << data_info_path << "] by origin order[" << origin_order << "]." << std::endl;
#endif
    auto start = std::chrono::steady_clock::now();

    auto graph_ptr = new GastCoCo::CBList(FLAGS_data, GastCoCo::ComputeMode::Mixed, FLAGS_o);

    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double, std::micro> elapsed = end - start;
    std::cout << "Building CBList takes time: " << elapsed.count() << "us" << std::endl;

    return graph_ptr;
}

void ExecuteAlgorithm
(
    std::string algorithm_name,
    std::string data_info_path, bool origin_order,
    bool compute_mode, bool execute_mode, bool prefetch_mode,
    int thread_num, int coroutine_num,
    int iterations, int batch_size
)
{
    if (algorithm_name == "pagerank" || algorithm_name == "pr")
    {
        if (compute_mode == false)
        {
            // GastCoCo::CBList graph(FLAGS_data, GastCoCo::ComputeMode::Mixed, FLAGS_o);
            // pagerank(graph, FLAGS_t, FLAGS_c, FLAGS_i);
        }
        else
        {
            // use pull only
            auto graph = BuildGraph(data_info_path, GastCoCo::ComputeMode::Pull, origin_order);
            pagerank(*graph, thread_num, coroutine_num, iterations);
        }
    }
    else if (algorithm_name == "sssp" || algorithm_name == "sp")
    {
        if (compute_mode == false)
        {
            // GastCoCo::CBList graph(FLAGS_data, GastCoCo::ComputeMode::Mixed, FLAGS_o);
            // pagerank(graph, FLAGS_t, FLAGS_c, FLAGS_i);
        }
        else
        {
            // use push only
            // GastCoCo::CBList graph(FLAGS_data, GastCoCo::ComputeMode::Pull, FLAGS_o);
            // sssp(graph, FLAGS_t, FLAGS_c, FLAGS_i);
        }
    }
}

int main(int argc, char** argv)
{
#ifdef LOGO
    PrintLOGO();
#endif
    google::SetVersionString("1.0");
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    ExecuteAlgorithm(
        FLAGS_app,
        FLAGS_data, FLAGS_o,
        FLAGS_cm, FLAGS_em, FLAGS_pm,
        FLAGS_t, FLAGS_c,
        FLAGS_i, FLAGS_bs
    );
}