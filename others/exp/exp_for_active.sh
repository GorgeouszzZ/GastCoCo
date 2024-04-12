#!/bin/bash

# test0="../toolkit/test_cmake ../../dataset/hollywood.txt_shuf 116050145 1139905 30 $1"
# $test0

# test1="../toolkit/test_cmake /root/lihongfu/dataset/testgraph.txt 44 9 20 $1"
# $test1
data="livejournal"
file="../exp/log1"
for b in {1..5}
do
    echo "---------${data}-----------" >> ${file}
    echo "pr" >> ${file}
    s="../gemini/pagerank ../../dataset/data/${data}/${data}.RSB ../../dataset/data/${data}/${data}.info 10"
    $s >> ${file}
    # echo "sp" >> ${file}
    # s="../gemini/sssp ../../dataset/data/${data}/${data}.RSB ../../dataset/data/${data}/${data}.info 9"
    # $s >> ${file}
done

# for data in "twitter" "livejournal" "usa-road" "com-friendster";
# do
#     echo "---------${data}-----------" >> ../exp/log
#     for coro in "2" "3" "4" "6" "8" "13" "15" "20" "40" "60" "100";
#     do
#       echo "cpr ${coro} ${data}" >> ../exp/log
#       s="../toolkit/parallel_copagerank ../../dataset/${data}/${data}.RS ../../dataset/${data}/${data}.info 30 ${coro}"
#       $s
#       echo "csp ${coro} ${data}" >> ../exp/log
#       s="../toolkit/parallel_coSSSP ../../dataset/${data}/${data}.RS ../../dataset/${data}/${data}.info 0 20 ${coro}"
#       $s
#     done
# done