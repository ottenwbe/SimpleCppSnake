#!/bin/bash

# Exit immediately if a command exits with a non-zero status.
set -e

# Define the build directory
BUILD_DIR="build"

# Clean the build directory to resolve CMake cache path mismatches
echo "--- Cleaning build directory ---"
rm -rf ${BUILD_DIR}

# Force Clang as the compiler
export CC=clang
export CXX=clang++

# 1. Install dependencies using Conan
# This generates the CMake toolchain file in the build directory.
echo "--- Installing dependencies with Conan ---"
conan install . --output-folder=${BUILD_DIR} --build=missing -c tools.system.package_manager:mode=install -c tools.system.package_manager:sudo=True

# Detect the toolchain file path (Conan 2.x layout can vary)
TOOLCHAIN_FILE="${BUILD_DIR}/generators/conan_toolchain.cmake"
if [ ! -f "$TOOLCHAIN_FILE" ]; then
    # Fallback for different Conan layouts
    TOOLCHAIN_FILE=$(find ${BUILD_DIR} -name "conan_toolchain.cmake" | head -n 1)
fi

if [ -z "$TOOLCHAIN_FILE" ]; then
    echo "Error: conan_toolchain.cmake not found in ${BUILD_DIR}."
    exit 1
fi

# 2. Configure the project with CMake
echo "--- Configuring with CMake using toolchain: $TOOLCHAIN_FILE ---"
cmake -S . -B ${BUILD_DIR} -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE" -DCMAKE_BUILD_TYPE=Release

# 3. Build the executable
echo "--- Building with CMake ---"
cmake --build ${BUILD_DIR}

echo "--- Build complete! ---"
echo "You can now run the game with: ./${BUILD_DIR}/SimpleCppSnake"
