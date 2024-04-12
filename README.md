# GastCoCo v0.1
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
- Linux/Unix
### 2.2 Setup
```shell
cd GastCoCo
mkdir build && cd build
cmake ..
make
```

## 3. How to Run
```shell
./toolkit/coSSSP [data] [datainfo] [thread] [coro] [source]
./toolkit/coPageRank [data] [datainfo] [thread] [coro]
./toolkit/coBatchInsert [data] [datainfo]
```