#!/bin/bash
# Build script for W-MBus Decryptor
echo "W-MBus Decryptor Build Script"
echo "============================="

check_deps() {
    echo "Checking dependencies..."
    if ! command -v cmake &> /dev/null; then
        echo "Error: CMake not found. Please install CMake."
        exit 1
    fi
    if ! pkg-config --exists openssl; then
        echo "Warning: OpenSSL not found via pkg-config."
        echo "Trying to build with system OpenSSL..."
    fi
}

build_project() {
    echo "Building project..."
    mkdir -p build
    cd build

    if cmake ..; then
        echo "CMake configuration successful"
    else
        echo "CMake configuration failed"
        exit 1
    fi

    if make -j$(nproc); then
        echo "Build successful!"
        echo "Executable: ./wmbus_decryptor"
    else
        echo "Build failed"
        exit 1
    fi
}

check_deps
build_project

echo "Build complete. Run './build/wmbus_decryptor' to test."
