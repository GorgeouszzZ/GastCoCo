namespace CoroGraph
{
    //TODO: auto match
    //TODO: binary search in node?  if threshold? 
#define CACHE_LINE_SIZE 64
// #define LV1CHUNK_NCNT 6
#define LV1CHUNK_NCNT 14 // for 2* cache line
#define CHUNK_LEVEL 1
#define LEAFCHUNK_LEVEL 2

// #define CACHE_LINE_MULT 8
// #define LEAF_CHUNK_NCNT 27
// #define INTER_NODE_PTR_CNT 19
// #define INTER_NODE_KEY_CNT 18


#define CACHE_LINE_MULT 4   
#define LEAF_CHUNK_NCNT 27
#define INTER_NODE_PTR_CNT 19
#define INTER_NODE_KEY_CNT 18


// #define CACHE_LINE_MULT 2   
// #define LEAF_CHUNK_NCNT 10
// #define INTER_NODE_PTR_CNT 9
// #define INTER_NODE_KEY_CNT 8
}