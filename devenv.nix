{ pkgs, lib, config, inputs, ... }:

let
  # Helper functions to handle build steps
  buildDebug = ''
    print_header() {
      echo -e "\033[0;34m=== $1 ===\033[0m"
    }

    print_success() {
      echo -e "\033[0;32m✓ $1\033[0m"
    }

    print_error() {
      echo -e "\033[0;31m✗ $1\033[0m"
    }

    BUILD_DIR="build"
    SRC_DIR="src"
    GENERATOR="Ninja"

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
      cppcheck --project=$BUILD_DIR/compile_commands.json \
               --enable=all \
               --check-level=exhaustive \
               --std=c11 \
               --quiet \
               --inline-suppr \
               --suppress=missingIncludeSystem \
               -i $BUILD_DIR \
               --suppress="*:*_tests.c" \
               --suppress=checkersReport \
               --library=posix.cfg
      if [ $? -eq 0 ]; then
        print_success "Cppcheck completed successfully"
      else
        print_error "Cppcheck found issues"
      fi
    }
  '';
  
in
{
  # Install necessary packages
  packages = [
    pkgs.git
    pkgs.clang
    pkgs.cmake
    pkgs.ninja
    pkgs.cppcheck
    pkgs.pkg-config
    pkgs.gdb   # optional
    pkgs.valgrind # optional
    pkgs.gcc    # optional
    pkgs.gnumake # instead of pkgs.make
  ];

  # Shell script that runs when you enter the shell
  enterShell = ''
    echo "Welcome to Sigil C Development Environment!"
    echo "Clang version:"
    clang --version

    echo "CMake version:"
    cmake --version

    # Expose the build functions to the shell
    $buildDebug
    echo "Ready to build, run, and test your project."
  '';

  # Optional: Test hook to verify the setup
  enterTest = ''
    echo "Testing clang and cmake availability..."
    clang --version
    cmake --version
  '';
}
