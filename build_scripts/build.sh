#!/bin/bash
BUILD_DIR="$(cd "$(dirname "$0")/.." && pwd)/build"
SRC_DIR="$(cd "$(dirname "$0")/.." && pwd)"

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

cmake -DCMAKE_BUILD_TYPE=Release "$SRC_DIR"
if [ $? -ne 0 ]; then exit 1; fi

make -j8
if [ $? -ne 0 ]; then exit 1; fi

echo "Build completed successfully!"