#!/bin/sh

# Build DJV against the system's packages rather than the super build, the
# way docs/building.html describes: lunasvg and subprocess.h, which have no
# packages, from source into a prefix beside the build, then DJV configured
# directly with that prefix on the search path. What to build is in etc/Config/*.cmake, as
# it is for the super build.
#
# Usage: sh build-system.sh <source directory> <build type> [config]

set -e
set -x

SOURCE_DIR=$1
BUILD_TYPE=$2
CONFIG=${3:-default}
CONFIG_FILE=$SOURCE_DIR/etc/Config/$CONFIG.cmake

if [ -z "$CMAKE_BUILD_PARALLEL_LEVEL" ]; then
    CMAKE_BUILD_PARALLEL_LEVEL=$(nproc)
    export CMAKE_BUILD_PARALLEL_LEVEL
fi

if [ ! -f $SOURCE_DIR/deps/tlRender/CMakeLists.txt ]; then
    git -C $SOURCE_DIR submodule update --init --recursive
fi

# The version the super build pins: see
# deps/tlRender/deps/ftk/etc/SuperBuild/cmake/Modules/Buildlunasvg.cmake.
if [ ! -d lunasvg ]; then
    git clone --branch v3.4.0 https://github.com/sammycage/lunasvg.git
fi
cmake \
    -S lunasvg \
    -B lunasvg-$BUILD_TYPE \
    -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
    -DCMAKE_INSTALL_PREFIX=$PWD/install-$BUILD_TYPE \
    -DLUNASVG_BUILD_EXAMPLES=OFF
cmake --build lunasvg-$BUILD_TYPE --config $BUILD_TYPE --target install

# A single header, at the commit the super build pins: see
# deps/tlRender/etc/SuperBuild/cmake/Modules/Buildsubprocess.cmake.
if [ ! -d subprocess ]; then
    git clone https://github.com/sheredom/subprocess.h.git subprocess
    git -C subprocess checkout 0d76f78ff8b56d1240ffe6571d689c5e26299527
fi
mkdir -p install-$BUILD_TYPE/include
cp subprocess/subprocess.h install-$BUILD_TYPE/include

# Ubuntu installs OpenColorIO's CMake configuration in /usr/share/cmake
# itself rather than a directory of its own there, which is not a place
# find_package() looks, so it is named. Packaging is off: it gathers the
# super build's libraries out of the install prefix.
cmake \
    -S $SOURCE_DIR \
    -B build-$BUILD_TYPE \
    -C $CONFIG_FILE \
    -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
    -DCMAKE_INSTALL_PREFIX=$PWD/install-$BUILD_TYPE \
    -DCMAKE_PREFIX_PATH=$PWD/install-$BUILD_TYPE \
    -DOpenColorIO_DIR=/usr/share/cmake \
    -DDJV_PACKAGE_DEPS=OFF
cmake --build build-$BUILD_TYPE --config $BUILD_TYPE
cmake --build build-$BUILD_TYPE --config $BUILD_TYPE --target install
