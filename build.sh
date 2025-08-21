#!/bin/bash

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

BUILD_DIR="build"
SRC_DIR="src"

print_header() {
    echo -e "${BLUE}=== $1 ===${NC}"
}

print_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

print_error() {
    echo -e "${RED}✗ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠ $1${NC}"
}

build_debug() {
    print_header "Building in Debug mode"
    cmake -S $SRC_DIR -B $BUILD_DIR -DCMAKE_BUILD_TYPE=Debug
    if [ $? -eq 0 ]; then
        cmake --build $BUILD_DIR -j$(nproc)
        if [ $? -eq 0 ]; then
            print_success "Debug build completed successfully"
        else
            print_error "Build failed"
            exit 1
        fi
    else
        print_error "CMake configuration failed"
        exit 1
    fi
}

build_release() {
    print_header "Building in Release mode with -O3"
    cmake -S $SRC_DIR -B $BUILD_DIR -DCMAKE_BUILD_TYPE=Release
    if [ $? -eq 0 ]; then
        cmake --build $BUILD_DIR -j$(nproc)
        if [ $? -eq 0 ]; then
            print_success "Release build completed successfully"
        else
            print_error "Build failed"
            exit 1
        fi
    else
        print_error "CMake configuration failed"
        exit 1
    fi
}

run_tests() {
    print_header "Building and running tests"
    build_debug
    print_header "Running tests"
    cd $BUILD_DIR && ctest --output-on-failure && cd ..
}

run_binary() {
    print_header "Building and running sigil binary"
    build_debug
    print_header "Running sigil"
    ./$BUILD_DIR/sigil
}

clean_build() {
    print_header "Cleaning build directory"
    rm -rf $BUILD_DIR
    print_success "Build directory cleaned"
}

case "$1" in
    "build")
        build_debug
        ;;
    "release")
        build_release
        ;;
    "test")
        run_tests
        ;;
    "run")
        run_binary
        ;;
    "clean")
        clean_build
        ;;
    "help"|"-h"|"--help")
        echo "Usage: ./build.sh [command]"
        echo ""
        echo "Commands:"
        echo "  build    - Build in debug mode (default)"
        echo "  release  - Build in release mode with -O3"
        echo "  test     - Build and run all tests"
        echo "  run      - Build and run the sigil binary"
        echo "  clean    - Clean the build directory"
        echo "  help     - Show this help message"
        ;;
    *)
        if [ -z "$1" ]; then
            build_debug
        else
            print_error "Unknown command: $1"
            echo "Use './build.sh help' for usage information"
            exit 1
        fi
        ;;
esac
