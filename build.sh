#!/bin/bash

BUILD_DIR=build
EXECUTABLE=sigil   # change as needed

function configure_build() {
  echo "Configuring build..."
  meson setup "$BUILD_DIR"
}

function build_project() {
  echo "Building project..."
  meson compile -C "$BUILD_DIR" --verbose
}

function clean_build() {
  echo "Cleaning build directory..."
  rm -rf "$BUILD_DIR"
}

function run_executable() {
  if [ ! -f "$BUILD_DIR/$EXECUTABLE" ]; then
    echo "Executable not found, building first..."
    configure_build
    build_project
  fi
  echo "Running $EXECUTABLE..."
  "$BUILD_DIR/$EXECUTABLE"
}

case "$1" in
  clean)
    clean_build
    ;;
  rebuild)
    clean_build
    configure_build
    build_project
    ;;
  run)
    run_executable
    ;;
  "")
    configure_build
    build_project
    ;;
  *)
    echo "Usage: $0 [clean|rebuild|run] [--verbose]"
    exit 1
    ;;
esac
