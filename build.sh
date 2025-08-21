# #!/bin/bash

# # Colors for output
# RED='\033[0;31m'
# GREEN='\033[0;32m'
# YELLOW='\033[1;33m'
# BLUE='\033[0;34m'
# NC='\033[0m' # No Color

# BUILD_DIR="build"
# SRC_DIR="src"

# print_header() {
#     echo -e "${BLUE}=== $1 ===${NC}"
# }

# print_success() {
#     echo -e "${GREEN}✓ $1${NC}"
# }

# print_error() {
#     echo -e "${RED}✗ $1${NC}"
# }

# print_warning() {
#     echo -e "${YELLOW}⚠ $1${NC}"
# }

# build_debug() {
#     print_header "Building in Debug mode"
#     cmake -S $SRC_DIR -B $BUILD_DIR -DCMAKE_BUILD_TYPE=Debug
#     if [ $? -eq 0 ]; then
#         cmake --build $BUILD_DIR -j$(nproc)
#         if [ $? -eq 0 ]; then
#             print_success "Debug build completed successfully"
#         else
#             print_error "Build failed"
#             exit 1
#         fi
#     else
#         print_error "CMake configuration failed"
#         exit 1
#     fi
# }

# build_release() {
#     print_header "Building in Release mode with -O3"
#     cmake -S $SRC_DIR -B $BUILD_DIR -DCMAKE_BUILD_TYPE=Release
#     if [ $? -eq 0 ]; then
#         cmake --build $BUILD_DIR -j$(nproc)
#         if [ $? -eq 0 ]; then
#             print_success "Release build completed successfully"
#         else
#             print_error "Build failed"
#             exit 1
#         fi
#     else
#         print_error "CMake configuration failed"
#         exit 1
#     fi
# }

# run_tests() {
#     print_header "Building and running tests"
#     build_debug
#     print_header "Running tests"
#     cd $BUILD_DIR && ctest --output-on-failure && cd ..
# }

# run_binary() {
#     print_header "Building and running sigil binary"
#     build_debug
#     print_header "Running sigil"
#     ./$BUILD_DIR/sigil
# }

# clean_build() {
#     print_header "Cleaning build directory"
#     rm -rf $BUILD_DIR
#     print_success "Build directory cleaned"
# }

# case "$1" in
#     "build")
#         build_debug
#         ;;
#     "release")
#         build_release
#         ;;
#     "test")
#         run_tests
#         ;;
#     "run")
#         run_binary
#         ;;
#     "clean")
#         clean_build
#         ;;
#     "help"|"-h"|"--help")
#         echo "Usage: ./build.sh [command]"
#         echo ""
#         echo "Commands:"
#         echo "  build    - Build in debug mode (default)"
#         echo "  release  - Build in release mode with -O3"
#         echo "  test     - Build and run all tests"
#         echo "  run      - Build and run the sigil binary"
#         echo "  clean    - Clean the build directory"
#         echo "  help     - Show this help message"
#         ;;
#     *)
#         if [ -z "$1" ]; then
#             build_debug
#         else
#             print_error "Unknown command: $1"
#             echo "Use './build.sh help' for usage information"
#             exit 1
#         fi
#         ;;
# esac

#!/bin/bash

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

BUILD_DIR="build"
SRC_DIR="src"
GENERATOR="Ninja"  # Changed to Ninja

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
    print_header "Building in Debug mode with Ninja"
    cmake -S $SRC_DIR -B $BUILD_DIR -DCMAKE_BUILD_TYPE=Debug -G "$GENERATOR"
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
    print_header "Building in Release mode with -O3 and Ninja"
    cmake -S $SRC_DIR -B $BUILD_DIR -DCMAKE_BUILD_TYPE=Release -G "$GENERATOR"
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

run_cppcheck() {
    print_header "Running Cppcheck (excluding tests)"
    if command -v cppcheck &> /dev/null; then
        cppcheck --project=$BUILD_DIR/compile_commands.json \
                 --enable=all \
                 --std=c11 \
                 --inline-suppr \
                 --suppress=missingIncludeSystem \
                 -i $BUILD_DIR \
                 --suppress="*:*_tests.c"
        if [ $? -eq 0 ]; then
            print_success "Cppcheck completed successfully"
        else
            print_warning "Cppcheck found issues"
        fi
    else
        print_error "Cppcheck not installed"
        exit 1
    fi
}

# New function to check if Ninja is available
check_ninja() {
    if ! command -v ninja &> /dev/null; then
        print_warning "Ninja not found. Installing..."
        # Try to install ninja (adapt for your package manager)
        if command -v apt-get &> /dev/null; then
            sudo apt-get install -y ninja-build
        elif command -v dnf &> /dev/null; then
            sudo dnf install -y ninja-build
        elif command -v pacman &> /dev/null; then
            sudo pacman -S --noconfirm ninja
        elif command -v brew &> /dev/null; then
            brew install ninja
        else
            print_error "Ninja not found and couldn't auto-install. Please install ninja-build package."
            exit 1
        fi
    fi
}

case "$1" in
    "build")
        check_ninja
        build_debug
        ;;
    "release")
        check_ninja
        build_release
        ;;
    "test")
        check_ninja
        run_tests
        ;;
    "run")
        check_ninja
        run_binary
        ;;
    "clean")
        clean_build
        ;;
    "cppcheck")
        run_cppcheck
        ;;
    "help"|"-h"|"--help")
        echo "Usage: ./build.sh [command]"
        echo ""
        echo "Commands:"
        echo "  build    - Build in debug mode with Ninja (default)"
        echo "  release  - Build in release mode with -O3 and Ninja"
        echo "  test     - Build and run all tests with Ninja"
        echo "  run      - Build and run the sigil binary with Ninja"
        echo "  clean    - Clean the build directory"
        echo "  help     - Show this help message"
        ;;
    *)
        if [ -z "$1" ]; then
            check_ninja
            build_debug
        else
            print_error "Unknown command: $1"
            echo "Use './build.sh help' for usage information"
            exit 1
        fi
        ;;
esac
