#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <array>
#include <assert.h>

namespace GastCoCo
{
struct Empty { };
using VertexID = uint32_t;
using EdgeID = uint32_t;
using IndexType = size_t;
using WeightType = uint32_t;
#define VertexSize 32 // sizeof(VertexID)
#define WeightSize 32 // sizeof(WeightType)

enum ThreadStatus
{
  WORKING,
  STEALING
};

struct ThreadState 
{
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
    VertexID start_vertex;
    VertexID end_vertex;
    WeightType value;
    Gedge(){};
    Gedge(VertexID src, VertexID dst, WeightType v):start_vertex(src),end_vertex(dst),value(v){};
};

struct Gedge_noWeight
{
    VertexID start_vertex;
    VertexID end_vertex;
    Gedge_noWeight(VertexID src, VertexID dst):start_vertex(src),end_vertex(dst){};
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
}