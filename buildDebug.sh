#!/bin/sh

set -xe

options="-Wall -Wextra -Werror -Wpedantic -g"

build_dir="debug"

if [ ! -d "$build_dir" ]; then
  echo "debug directory does not exist. Creating..."
  mkdir "$build_dir"
else
  echo "debug directory already exists."
fi

clang $options -o ./debug/debug main.c
