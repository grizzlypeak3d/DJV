#!/bin/sh

# Usage: sh package-linux.sh [source directory] [build type]
#
# Builds with the "package-linux" config and then makes the package. What to
# build is in etc/Config/package-linux.cmake, not here.

set -e

SOURCE_DIR=${1:-DJV}
BUILD_TYPE=${2:-Release}

sh $SOURCE_DIR/etc/Linux/sbuild.sh $SOURCE_DIR $BUILD_TYPE package-linux
cmake --build build-$BUILD_TYPE --config $BUILD_TYPE --target package
