#include <vector>
#include <stdint.h>
#include <iostream>

[[nodiscard("SliceTaskForCoroResult")]]std::vector<int32_t> SliceTaskForN(int Task_num, uint32_t N)
{
    std::vector<int32_t> Result(N+1, 0);
    Result[N] = Task_num;
    int avg_task = Task_num / N;
    for(int i=1;i<N;++i)
        Result[i] = avg_task + Result[i-1];
    // for(auto& i:Result)
    //     std::cout<<i<<"-";
    return Result;
}