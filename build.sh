#!/bin/bash
set -e

echo "Building image2pixel..."

if ! command -v cmake &> /dev/null; then
    echo "Error: cmake is not installed."
    exit 1
fi

BUILD_DIR="cmake-build-release"

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure
cmake -DCMAKE_BUILD_TYPE=Release ..

# Build
cmake --build .

echo "--------------------------------------------------------"
echo "Build complete."
if [ -f "image2pixel.exe" ]; then
    echo "Executable is at $BUILD_DIR/image2pixel.exe"
elif [ -f "image2pixel" ]; then
    echo "Executable is at $BUILD_DIR/image2pixel"
fi
echo "--------------------------------------------------------"