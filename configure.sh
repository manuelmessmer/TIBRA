#!/bin/bash

# Set compiler (optional)
export CC=gcc
export CXX=g++

export APP_SOURCE="${APP_SOURCE:-"$( cd "$(dirname "$0")" ; pwd -P )"}/tibra"
export APP_BUILD="${APP_SOURCE}/../build/"

# Set basic configuration
export CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE:-"Release"}
export PYTHON_EXECUTABLE=${PYTHON_EXECUTABLE:-"/usr/bin/python3"}

# Clean
clear
rm -rf "${APP_BUILD}/${CMAKE_BUILD_TYPE}/cmake_install.cmake"
rm -rf "${APP_BUILD}/${CMAKE_BUILD_TYPE}/CMakeCache.txt"

# Configure
cmake -H"${APP_SOURCE}" -B"${APP_BUILD}/${CMAKE_BUILD_TYPE}" \
    -DUSE_CGAL=OFF \
    -DCGAL_DIR="..." \

# Buid
cmake --build "${APP_BUILD}/${CMAKE_BUILD_TYPE}" --target install -- -j$(nproc)