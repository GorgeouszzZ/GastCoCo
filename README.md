# GastCoCo v1.0
<!-- ![GastCoCo](GastCoCo.webp) -->
<div align=left> <img src="./others/GastCoCo.webp" width = 30%/> </div>

## 1. Introduction
This repo contains all the source code to build GastCoCo.
(GastCoCo: Graph Storage and Coroutine-Based Prefetch
Co-Design for Dynamic Graph Processing)

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
mkdir build && cd build
cmake .. --DCMAKE_BUILD_TYPE=Release
make
```

## 3. How to Run
```shell
# old version
./toolkit/coSSSP [data] [datainfo] [thread] [coro] [source]
./toolkit/coPageRank [data] [datainfo] [thread] [coro] [iter]
./toolkit/coBatchInsert [data] [datainfo] [thread] [coro] [batchsize]
# rebuilding for new
./gastcoco --app=pagerank --t=[thread] --c=[coro] --i=[iter] --data=[datainfo] --cm
```

## 4. Dataset
[data] format is [src-vertex dst-vertex weight]. By default, GastCoCo loads binary data files. Once you have prepared the [data], you can use "GastCoCo/others/datatoolkit/TransBinary" to convert it into a binary file, which will speed up data loading. If you prefer not to do this, you can modify the CBList constructor to use other data loading functions provided in "GastCoCo/other/graphIO.hpp."

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
