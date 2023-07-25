#!/bin/sh

set -xe

options="-Wall -Wextra -Werror -Wpedantic"

build_dir="build"

if [ ! -d "$build_dir" ]; then
  echo "Build directory does not exist. Creating..."
  mkdir "$build_dir"
else
  echo "Build directory already exists."
fi

clang $options -o ./build/main main.c
