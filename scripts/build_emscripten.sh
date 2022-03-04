#!/bin/bash -e

# Build a debug version with emscripten.

SCRIPT_DIR=`dirname $0`
cd "$SCRIPT_DIR/.."

mkdir -p build
cd build

core_count=`getconf _NPROCESSORS_ONLN`
emcmake cmake ..
emmake make -j `expr $core_count + 1`
