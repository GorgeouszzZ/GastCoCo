#pragma once
#include "../CBList/CBList.hpp"
#include <vector>

[[nodiscard("SliceForCoroResult")]]std::vector<uint32_t> SliceForCoro(const CoroGraph::CBList& cbl, uint32_t coro_num)
{
    std::vector<uint32_t> ChunkNumRecord(cbl.NodeNum+1, 0);
    for(VertexID node=0; node<cbl.NodeNum; ++node)
    {
        ChunkNumRecord[node+1] = ChunkNumRecord[node] + cbl.NodeList[node].ChunkCnt;
    }
    uint32_t chunk_num_for_coro = ChunkNumRecord[cbl.NodeNum]/coro_num;
    uint32_t chunk_sum = 0;
    std::vector<uint32_t> SliceResult(coro_num+1, 0);
    uint32_t tid = 1;
    for(uint32_t i=1; i<cbl.NodeNum+1; ++i)
    {
        if(ChunkNumRecord[i]>=chunk_sum + chunk_num_for_coro)
        {
            if(tid==1 || ChunkNumRecord[i-1]==SliceResult[tid-1] || ChunkNumRecord[i]-chunk_sum > chunk_sum-ChunkNumRecord[i-1])
            {
                SliceResult[tid] = i;
                chunk_sum = ChunkNumRecord[i];
                ++tid;
            }
            else
            {
                SliceResult[tid] = i-1;
                chunk_sum = ChunkNumRecord[i-1];
                ++tid;
                --i;
            }
        }
        if(tid == coro_num)
        {
            SliceResult[tid] = cbl.NodeNum;
            break;
        }
    }
    // for(auto &i : SliceResult)
    //     printf(" %d",i);
    return SliceResult;
}

[[nodiscard("SliceForCoroDynamicResult")]]std::vector<uint32_t> SliceForCoroDynamic(const CoroGraph::CBList& cbl, uint32_t coro_num)
{
    std::vector<uint32_t> ChunkNumRecord(cbl.NodeNum+1, 0);
    for(VertexID node=0; node<cbl.NodeNum; ++node)
    {
        ChunkNumRecord[node+1] = ChunkNumRecord[node] + cbl.NodeList[node].ChunkCnt;
    }
    uint32_t chunk_num_for_coro = ChunkNumRecord[cbl.NodeNum]/coro_num;
    uint32_t chunk_sum = 0;
    std::vector<uint32_t> SliceResult(coro_num+1, 0);
    uint32_t tid = 1;
    for(uint32_t i=1; i<cbl.NodeNum+1; ++i)
    {
        if(ChunkNumRecord[i]>=chunk_sum + chunk_num_for_coro)
        {
            if(tid==1 || ChunkNumRecord[i-1]==SliceResult[tid-1] || ChunkNumRecord[i]-chunk_sum > chunk_sum-ChunkNumRecord[i-1])
            {
                SliceResult[tid] = i;
                // std::cout<<"i:"<<i<<" tid:"<<tid<<"-"<<SliceResult[tid]<<std::endl;
                chunk_sum = ChunkNumRecord[i];
                ++tid;
            }
            else
            {
                SliceResult[tid] = i-1;
                // std::cout<<"i:"<<i<<" tid:"<<tid<<"-"<<SliceResult[tid]<<std::endl;
                chunk_sum = ChunkNumRecord[i-1];
                ++tid;
                --i;
            }
        }
        if(tid == coro_num)
        {
            SliceResult[tid] = cbl.NodeNum;
            // std::cout<<"i:"<<i<<" tid:"<<tid<<"-"<<SliceResult[tid]<<std::endl;
            break;
        }
        else if(i == cbl.NodeNum)
        {
            // std::cout<<"i:"<<i<<" tid:"<<tid<<"-"<<SliceResult[tid]<<std::endl;
            for(;tid<=coro_num;++tid)
                SliceResult[tid] = cbl.NodeNum;
        }
    }
    // for(auto &i : SliceResult)
    //     printf(" %d",i);
    return SliceResult;
}

[[nodiscard("SliceTaskForCoroResult")]]std::vector<int32_t> SliceTaskForCoro(int Task_num, uint32_t coro_num)
{
    std::vector<int32_t> Result(coro_num+1, 0);
    Result[coro_num] = Task_num;
    int avg_task = Task_num / coro_num;
    for(int i=1;i<coro_num;++i)
        Result[i] = avg_task + Result[i-1];
    // for(auto& i:Result)
    //     std::cout<<i<<"-";
    return Result;
}

[[nodiscard("SliceForVerticesResult")]]std::vector<int32_t> SliceForVertices(int Task_num, uint32_t thread_num, int basic_chunk)
{
    std::vector<int32_t> Result(thread_num+1, 0);
    Result[thread_num] = Task_num;
    int chunknum = Task_num / basic_chunk + (Task_num%basic_chunk == 0? 0:1);
    int avg_chunk = chunknum / thread_num;
    for(int i=1;i<thread_num;++i)
        Result[i] = avg_chunk*basic_chunk + Result[i-1];
    // for(auto& i:Result)
    //     std::cout<<i<<"-";
    return Result;
}

[[nodiscard("SliceForAverageResult")]]std::vector<int32_t> SliceForAverage(int slice_num, int left, int right)
{
    std::vector<int32_t> Result(slice_num+1, 0);
    Result[slice_num] = right + 1;
    Result[0] = left;
    int avg_chunk = (right - left + 1) / slice_num;
    for(int i=1;i<slice_num;++i)
        Result[i] = avg_chunk + Result[i-1];
    // for(auto& i:Result)
    //     std::cout<<i<<"-";
    return Result;
}

[[nodiscard("SliceForBatchInsertResult")]]std::vector<int32_t> SliceForBatchInsert(int slice_num, int left, int right)
{
    std::vector<int32_t> Result(slice_num+1, 0);
    Result[slice_num] = right + 1;
    Result[0] = left;
    int avg_chunk = (right - left + 1) / slice_num;
    for(int i=1;i<slice_num;++i)
        Result[i] = avg_chunk + Result[i-1];
    // for(auto& i:Result)
    //     std::cout<<i<<"-";
    return Result;
}

bool slice_for_coro(int* &result, int coro_num, CoroGraph::CBList cbl)
{
    result = new int [coro_num + 1];
    int* silce_num = new int [coro_num];
    int chunk_sum = 0;
    for(int i=0;i<cbl.NodeNum;i++)
    {
        chunk_sum+=cbl.NodeList[i].ChunkCnt;
    }
    int delta = chunk_sum/coro_num;
    result[0] = 0;
    result[coro_num] = cbl.NodeNum;
    int tmp = 0;
    int coro_cnt = 1;
    for(int i=0;i<cbl.NodeNum;i++)
    {
        tmp+=cbl.NodeList[i].ChunkCnt;
        if(tmp > delta)
        {
            result[coro_cnt] = i;
            silce_num[coro_cnt - 1] = tmp - cbl.NodeList[i].ChunkCnt;
            coro_cnt++;
            tmp = cbl.NodeList[i].ChunkCnt;
        }
    }
    result[coro_num] = cbl.NodeNum;
    for(int i=0;i<coro_num+1;i++)
    {
        printf("%d ",result[i]);
    }
    printf("\n");
    return true;
}


int scoreLv1 = 1;
int scoreLeaf = 4;
[[nodiscard("SliceForCoroResultWithScore")]]std::vector<int32_t> SliceForCoroWithScore(const CoroGraph::CBList& cbl, uint32_t coro_num)
{
    std::vector<uint32_t> ChunkScoreRecord(cbl.NodeNum+1, 0);
    for(VertexID node=0; node<cbl.NodeNum; ++node)
    {
        if(cbl.NodeList[node].BPT.Root == nullptr)
            ChunkScoreRecord[node+1] = (ChunkScoreRecord[node] + cbl.NodeList[node].ChunkCnt * scoreLv1);
        else
            ChunkScoreRecord[node+1] = (ChunkScoreRecord[node] + cbl.NodeList[node].ChunkCnt * scoreLeaf);
    }
    uint32_t Score_for_coro = ChunkScoreRecord[cbl.NodeNum]/coro_num;
    uint32_t score_now = 0;
    std::vector<int32_t> SliceResult(coro_num+1, 0);
    uint32_t tid = 1;
    for(uint32_t i=1; i<cbl.NodeNum+1; ++i)
    {
        if(ChunkScoreRecord[i]>=score_now + Score_for_coro)
        {
            if(tid==1 || ChunkScoreRecord[i-1]==SliceResult[tid-1] || ChunkScoreRecord[i]-score_now > score_now-ChunkScoreRecord[i-1])
            {
                SliceResult[tid] = i;
                std::cout<<"i:"<<i<<" tid:"<<tid<<"-"<<SliceResult[tid]<<std::endl;
                score_now = ChunkScoreRecord[i];
                ++tid;
            }
            else
            {
                SliceResult[tid] = i-1;
                std::cout<<"i:"<<i<<" tid:"<<tid<<"-"<<SliceResult[tid]<<std::endl;
                score_now = ChunkScoreRecord[i-1];
                ++tid;
                --i;
            }
        }
        if(tid == coro_num)
        {
            SliceResult[tid] = cbl.NodeNum;
            std::cout<<"i:"<<i<<" tid:"<<tid<<"-"<<SliceResult[tid]<<std::endl;
            break;
        }
        else if(i == cbl.NodeNum)
        {
            std::cout<<"i:"<<i<<" tid:"<<tid<<"-"<<SliceResult[tid]<<std::endl;
            for(;tid<=coro_num;++tid)
                SliceResult[tid] = cbl.NodeNum;
        }
    }
    // for(auto &i : SliceResult)
    //     printf(" %d",i);
    return SliceResult;
}


