#!/usr/bin/env bash

# enable err check
set -e

echo "🛠️  Starting build GastCoCo..."
if [ ! -d "build" ]; then
    echo "📂 Creating build directory: build/"
    mkdir build
else
    echo "📂 Existing build directory detected, using it."
fi

cd build
echo "🔧 Generating build GastCoCo (CMake)..."
# DCM
dcm="$1"
case "$dcm" in
    0|1|2|4|8|16) ;;
    *) dcm=0 ;;
esac
# build mode
bm="$2"
case "${bm,,}" in
    release) bm="Release" ;;
    debug)   bm="Debug"   ;;
    *)       bm="Release" ;;
esac
echo "Using [$dcm] integer multiple of the cache line size on CBList."
echo "Using [$bm] mode"
disable_numa_alloc="OFF"
if grep -qiE "microsoft|wsl" /proc/sys/kernel/osrelease 2>/dev/null; then
    disable_numa_alloc="ON"
fi
echo "Disable NUMA coroutine arena allocation: [$disable_numa_alloc]"
cmake -DCM="$dcm" -DCMAKE_BUILD_TYPE="$bm" -DGASTCOCO_DISABLE_NUMA_ALLOC="$disable_numa_alloc" ..

echo "🔨 Compiling GastCoCo..."
make

echo "✅ Build completed!"
