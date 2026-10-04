#!/bin/sh

# Usage: sh package-macos.sh [source directory] [build type] [architecture]
#
# Builds with the "package-macos" config and then makes the package. What to
# build is in etc/Config/package-macos.cmake, not here.
#
# The architecture is arm64, or x64 for Intel Macs, which builds with the
# "package-macos-x64" config. The two are separate packages, and each is
# built in a directory of its own: see etc/Config/package-macos-x64.cmake.

set -e

SOURCE_DIR=${1:-DJV}
BUILD_TYPE=${2:-Release}
case "${3:-arm64}" in
    arm64) CONFIG=package-macos ;;
    x64|x86_64) CONFIG=package-macos-x64 ;;
    *) echo "Unknown architecture: $3 (arm64 or x64)" >&2; exit 1 ;;
esac

sh $SOURCE_DIR/etc/macOS/sbuild.sh $SOURCE_DIR $BUILD_TYPE $CONFIG
cmake --build build-$BUILD_TYPE --config $BUILD_TYPE --target package
