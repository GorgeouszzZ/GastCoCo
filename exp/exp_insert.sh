file="../exp/log"

s="../toolkit/parallel_Insert ../../dataset/data/com-friendster/com-friendster.RSB ../../dataset/data/com-friendster/com-friendster.info 50"
$s >> $file

s="../toolkit/parallel_coInsert ../../dataset/data/com-friendster/com-friendster.RSB ../../dataset/data/com-friendster/com-friendster.info 50 20"
$s >> $file