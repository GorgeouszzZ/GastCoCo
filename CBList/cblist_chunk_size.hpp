namespace GastCoCo
{
    //TODO: auto match
    //TODO: binary search in node?  if threshold? 

#define CHUNK_LEVEL 1
#define LEAFCHUNK_LEVEL 2
#define CACHE_LINE_SIZE 64

#if CM == 1
    #define CACHE_LINE_L1_MULT 1
    #define CACHE_LINE_MULT 1
    #define LV1CHUNK_NCNT 6
    #define LEAF_CHUNK_NCNT 3
    #define INTER_NODE_PTR_CNT 3
#elif CM == 2
    #define CACHE_LINE_L1_MULT 2
    #define CACHE_LINE_MULT 2
    #define LV1CHUNK_NCNT 14
    #define LEAF_CHUNK_NCNT 11
    #define INTER_NODE_PTR_CNT 9
#elif CM == 4
    #define CACHE_LINE_L1_MULT 4
    #define CACHE_LINE_MULT 4
    #define LV1CHUNK_NCNT 30
    #define LEAF_CHUNK_NCNT 27
    #define INTER_NODE_PTR_CNT 19
#elif CM == 8
    #define CACHE_LINE_L1_MULT 8
    #define CACHE_LINE_MULT 8
    #define LV1CHUNK_NCNT 62
    #define LEAF_CHUNK_NCNT 59
    #define INTER_NODE_PTR_CNT 41
#elif CM == 16
    #define CACHE_LINE_L1_MULT 16
    #define CACHE_LINE_MULT 16
    #define LV1CHUNK_NCNT 126
    #define LEAF_CHUNK_NCNT 123
    #define INTER_NODE_PTR_CNT 83
#else   //default chunk size
    #define CACHE_LINE_L1_MULT 2
    #define CACHE_LINE_MULT 4
    #define LV1CHUNK_NCNT 14
    #define LEAF_CHUNK_NCNT 27
    #define INTER_NODE_PTR_CNT 19
#endif
#define INTER_NODE_KEY_CNT INTER_NODE_PTR_CNT-1

}