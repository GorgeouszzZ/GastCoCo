#! /bin/bash
echo "------------view head-------------"
head -n 5 $1

echo "------------view tail-------------"
head -n 5 $1

echo -n "total lines:"
wc -l $1