cmake_minimum_required(VERSION 3.1)

set(CMAKE_C_COMPILE /usr/bin/gcc-10)
set(CMAKE_CXX_COMPILE /usr/bin/g++-10)

project(CoroGraph)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../toolkit/)


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -rdynamic -std=c++20 -fcoroutines -fno-exceptions -fopenmp -O3")
set(LINK_FLAGS "-lnuma")
link_libraries(${LINK_FLAGS})

# add_executable(parallel_copagerank workload/parallel_copagerank.cpp)
# add_executable(parallel_pagerank workload/parallel_pagerank.cpp)
# add_executable(parallel_coSSSP workload/parallel_coSSSP.cpp)
# add_executable(parallel_SSSP workload/parallel_SSSP.cpp)
# add_executable(parallel_coInsert workload/parallel_coInsert.cpp)
# add_executable(parallel_Insert workload/parallel_Insert.cpp)
# add_executable(coInsert workload/CBList_coInsert.cpp)
# add_executable(parallel_COPR workload/parallel_COPR.cpp)
add_executable(parallel_batch_insert workload/parallel_batch_insert.cpp)
add_executable(parallel_batch_coInsert workload/parallel_batch_coInsert.cpp)
# add_executable(CBList_coInsert workload/CBList_coInsert.cpp)
# add_executable(CBList_Insert workload/CBList_Insert.cpp)


# add_executable(gui testgflags/get_user_input.cpp)
# target_link_libraries(gui  libgflags.a libglog.a)
