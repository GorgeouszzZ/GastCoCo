cmake_minimum_required(VERSION 3.1)

set(CMAKE_C_COMPILE /usr/bin/gcc-10)
set(CMAKE_CXX_COMPILE /usr/bin/g++-10)

project(DataProcess)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -fopenmp")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../../datatoolkit/)



# add_executable(test_cmake workload/parallel_copagerank.cpp)
add_executable(TransBinary TransBinary.cpp)
target_link_libraries(TransBinary  libgflags.a libglog.a)
