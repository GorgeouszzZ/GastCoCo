#! /bin/bash

# test0="../toolkit/test_cmake ../../dataset/hollywood.txt_shuf 116050145 1139905 30 $1"
# $test0

# test1="../toolkit/test_cmake /root/lihongfu/dataset/testgraph.txt 44 9 20 $1"
# $test1
for data in "twitter" "livejournal" "usa-road" "com-friendster";
do
    echo "---------${data}-----------" >> ../exp/log
    echo "pr" >> ../exp/log
    s="../toolkit/parallel_pagerank ../../dataset/${data}/${data}.RS ../../dataset/${data}/${data}.info 30"
    $s
    echo "sp" >> ../exp/log
    s="../toolkit/parallel_SSSP ../../dataset/${data}/${data}.RS ../../dataset/${data}/${data}.info 0 20"
    $s
done

for data in "twitter" "livejournal" "usa-road" "com-friendster";
do
    echo "---------${data}-----------" >> ../exp/log
    for coro in "2" "3" "4" "6" "8" "13" "15" "20" "40" "60" "100";
    do
      echo "cpr ${coro} ${data}" >> ../exp/log
      s="../toolkit/parallel_copagerank ../../dataset/${data}/${data}.RS ../../dataset/${data}/${data}.info 30 ${coro}"
      $s
      echo "csp ${coro} ${data}" >> ../exp/log
      s="../toolkit/parallel_coSSSP ../../dataset/${data}/${data}.RS ../../dataset/${data}/${data}.info 0 20 ${coro}"
      $s
    done
done