#include <iostream>

#include "Parameter_input.hpp"
int main(int argc, char ** argv)
{
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    std::cout << "hello" << app_dummy << FLAGS_t << std::endl;
}