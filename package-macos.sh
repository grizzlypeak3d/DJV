#!/bin/sh

# Usage: sh package-macos.sh [source directory] [build type]
#
# Builds with the "package-macos" config and then makes the package. What to
# build is in etc/Config/package-macos.cmake, not here.

set -e

SOURCE_DIR=${1:-DJV}
BUILD_TYPE=${2:-Release}

sh $SOURCE_DIR/etc/macOS/sbuild.sh $SOURCE_DIR $BUILD_TYPE package-macos
cmake --build build-$BUILD_TYPE --config $BUILD_TYPE --target package
