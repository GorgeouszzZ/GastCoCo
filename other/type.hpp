#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <array>
#include <assert.h>

struct Empty { };
using VertexID = uint32_t;
using EdgeID = uint32_t;
using IndexType = size_t;
using WeightType = uint32_t;
#define VertexSize 32 // sizeof(VertexID)
#define WeightSize 32 // sizeof(WeightType)

enum ThreadStatus {
  WORKING,
  STEALING
};

struct ThreadState {
  VertexID curr;
  VertexID end;
  ThreadStatus status;
};

#define BLOCK_SIZE 1

#define random(x) (uint32_t)(rand()%x + 1)

struct AdjUnit
{
    VertexID dest;
    WeightType value;
}__attribute__((packed));

struct Gedge
{
    VertexID start_point;
    VertexID end_point;
    WeightType value;
    Gedge(){};
    Gedge(VertexID src, VertexID dst, WeightType v):start_point(src),end_point(dst),value(v){};
};

struct Gedge_noWeight
{
    VertexID start_point;
    VertexID end_point;
    Gedge_noWeight(VertexID src, VertexID dst):start_point(src),end_point(dst){};
};

// template <class T>
// struct edgeArray {
//   Gedge_noWeight* E;
//   T numRows;
//   T numCols;
//   T nonZeros;
//   void del() {free(E);}
//   edgeArray(Gedge_noWeight* EE, T r, T c, T nz) :
//     E(EE), numRows(r), numCols(c), nonZeros(nz) {}
//   edgeArray() {}
// };
