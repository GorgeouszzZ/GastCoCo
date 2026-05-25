# <div align=center> GastCoCo v1.0 Building... </div>
<!-- ![GastCoCo](GastCoCo.webp) -->
<div align=center> <img src="./others/GastCoCo.webp" width = 50%/> </div>

## 1. Introduction
This repo contains all the source code to build GastCoCo.
(GastCoCo: Graph Storage and Coroutine-Based Prefetch
Co-Design for Dynamic Graph Processing [VLDB2025])

## 2. Installation
### 2.1 Software Requirements
- GCC >= 10.3.0
- NUMA
- CMAKE >= 3.14
- gflags >= 2.2.2
- Linux/Unix
### 2.2 Setup
```shell
cd GastCoCo
bash build.sh
```

`build.sh` accepts two optional parameters:

```shell
bash build.sh [CM] [BUILD_TYPE]
```

- `CM`: integer multiple of the cache line size used by CBList chunks. Valid values are `0`, `1`, `2`, `4`, `8`, and `16`. Invalid or omitted values fall back to `0`, which uses the default setting.
- `BUILD_TYPE`: CMake build type. Valid values are `release` and `debug`, case-insensitive. Invalid or omitted values fall back to `Release`.

`build.sh` also configures the CMake option `GASTCOCO_DISABLE_NUMA_ALLOC` automatically:

- On WSL, it passes `-DGASTCOCO_DISABLE_NUMA_ALLOC=ON` to avoid unsupported NUMA `mbind` calls for coroutine arena allocation.
- On regular Linux systems, it passes `-DGASTCOCO_DISABLE_NUMA_ALLOC=OFF`, preserving NUMA allocation for performance runs.

When invoking CMake manually, set this option explicitly if needed.

Examples:

```shell
bash build.sh
bash build.sh 4 release
bash build.sh 0 debug
```

```shell
mkdir build && cd build
# CM is used to specify that the chunk size in CBList is an integer multiple of the cache line size.
# If set to 0 or left unspecified, the default setting will be used.
cmake -DCM=[0/1/2/4/8/16] -DCMAKE_BUILD_TYPE=Release -DGASTCOCO_DISABLE_NUMA_ALLOC=OFF ..
make
```

## 3. How to Run
```shell
# old version
./toolkit/coSSSP [data] [datainfo] [thread] [coro] [source]
./toolkit/coPageRank [data] [datainfo] [thread] [coro] [iter]
./toolkit/coBatchInsert [data] [datainfo] [thread] [coro] [batchsize]
# rebuilding for new
./gastcoco --app=pagerank --t=[thread] --c=[coro] --i=[iter] --data=[datainfo] --gm
# --gm enables graph-mode execution. For PageRank, GastCoCo loads mixed graph storage and computes by pull traversal.
./pr [datainfo] [thread] [iter]
# parameter flag introduction
./gastcoco --help
# version
./gastcoco --version
```

## 4. Dataset
[data] format is [src-vertex dst-vertex weight]. By default, GastCoCo loads binary data files. Once you have prepared the [data], you can use "GastCoCo/others/datatoolkit/TransBinary" to convert it into a binary file, which will speed up data loading. If you prefer not to do this, you can modify the CBList constructor to use other data loading functions provided in "GastCoCo/other/graph_io.hpp."

[datainfo] format is [vertex-num edge-num].

```shell
[data] // Binary
0 1 300
1 2 100
3 4 50

[datainfo]
5 3

#rebuilding for new
[datainfo]
5 3
./data
1
```
