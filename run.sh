#!/usr/bin/env bash

set -e

cd "$(dirname "$0")/build"

./gastcoco --app=pr --t=16 --c=2 --i=30 --data=/home/lihongfu/dataset/hollywood.info --cm
