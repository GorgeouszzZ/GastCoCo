#! /bin/bash

for coro in "2" "3" "4" "5" "6" "7" "8" "9" "10" "11" "12" "13" "14" "15" "16" "17" "18" "19" "20";
do
    echo "cpr ${coro} ${data}" >> ../exp/log
    s="../toolkit/parallel_copagerank ../../dataset/hollywood.txt_shuf ../../dataset/hollywood.info 30 ${coro}"
    $s >> ../exp/log
done
