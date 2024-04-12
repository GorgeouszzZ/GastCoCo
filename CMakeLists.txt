cmake_minimum_required(VERSION 3.1)

set(CMAKE_C_COMPILE /usr/bin/gcc-10)
set(CMAKE_CXX_COMPILE /usr/bin/g++-10)

project(CoroGraph)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../toolkit/)


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -rdynamic -std=c++20 -fcoroutines -fno-exceptions -fopenmp -O3")
set(LINK_FLAGS "-lnuma")
link_libraries(${LINK_FLAGS})

add_executable(coPageRank workload/coPageRank.cpp)
add_executable(coSSSP workload/coSSSP.cpp)
add_executable(coBatchInsert workload/coBatchInsert.cpp)

