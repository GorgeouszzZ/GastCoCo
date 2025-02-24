#!/usr/bin/env bash

# 启用严格错误检查
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
cmake ..

echo "🔨 Compiling GastCoCo..."
make

echo "✅ Build completed!"