cmake_minimum_required(VERSION 3.1)

set(CMAKE_C_COMPILE /usr/bin/gcc-10)
set(CMAKE_CXX_COMPILE /usr/bin/g++-10)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

project(GastCoCo)

if(DEFINED CM)
    add_definitions(-DCM=${CM})
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcoroutines -fno-exceptions -fopenmp -O3 -DLOGO")
set(LINK_FLAGS "-lnuma")
link_libraries(${LINK_FLAGS})

add_executable(gastcoco core/gastcoco.cpp)
target_link_libraries(gastcoco  libgflags.a libglog.a)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Adding DEBUG macro for gastcoco")
    target_compile_definitions(gastcoco PRIVATE DEBUG)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -rdynamic")
endif()

if(DEFINED CM)
    target_compile_definitions(gastcoco PRIVATE CM=${CM})
    message(STATUS "Set ${CM} multiple of the cache line size for gastcoco")
endif()