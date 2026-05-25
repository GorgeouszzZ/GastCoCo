#!/usr/bin/env bash

set -e

repo_root="$(cd "$(dirname "$0")/.." && pwd)"

thread_num="${1:-16}"
coro_num="${2:-2}"
iter_num="${3:-30}"
data_info="${4:-/home/lihongfu/dataset/hollywood.info}"

cmd=(
    "$repo_root/build/gastcoco"
    --app=pr
    --t="$thread_num"
    --c="$coro_num"
    --i="$iter_num"
    --data="$data_info"
    --gm
)

echo "+ ${cmd[*]}"
"${cmd[@]}"
