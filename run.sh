#!/usr/bin/env bash

set -e

cd "$(dirname "$0")/build"

run_cmd() {
    echo "+ $*"
    "$@"
}

run_cmd ./gastcoco --app=pr --t=16 --c=2 --i=30 --data=/home/lihongfu/dataset/hollywood.info --gm
run_cmd ./pr /home/lihongfu/dataset/hollywood.info 16 30
