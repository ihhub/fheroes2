#!/bin/bash

# Function to get number of CPU cores based on OS
get_cpu_cores() {
    if [[ "$OSTYPE" == "darwin"* ]]; then
        # macOS
        sysctl -n hw.ncpu
    else
        # Linux and others
        nproc
    fi
}

# Function to attempt build with Docker
build_with_docker() {
    echo "Attempting build with Docker fallback..."
    docker run --rm -v "$(pwd):/src" -w /src emscripten/emsdk emmake make -f Makefile.emscripten -j$(get_cpu_cores)
}

# Function to perform native build
perform_native_build() {
    echo "Starting native build process..."
    emmake make -f Makefile.emscripten -j$(get_cpu_cores)
}

# Main build process
if ! perform_native_build; then
    echo "Native build failed, attempting Docker fallback..."
    build_with_docker
fi
