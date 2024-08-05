#include "../CBList/CBListChunkSize.hpp"

namespace GastCoCo
{
inline void prefetch(const void* ptr) 
{
    typedef struct { char x[CACHE_LINE_SIZE]; } cacheline_t;
    asm volatile("prefetcht0 %0" : : "m"(*(const cacheline_t*)ptr));
  //__builtin_prefetch(*(const cacheline_t*)ptr));
}

inline void prefetch_Chunk(const void* ptr)
{
    for (int i = 0; i < CACHE_LINE_L1_MULT * CACHE_LINE_SIZE; i += CACHE_LINE_SIZE)
      prefetch((const char*)ptr + i);
}

inline void prefetch_TreeNode(const void* ptr)
{
    for (int i = 0; i < CACHE_LINE_MULT * CACHE_LINE_SIZE; i += CACHE_LINE_SIZE)
      prefetch((const char*)ptr + i);
}

}