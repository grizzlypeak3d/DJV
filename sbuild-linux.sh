#!/bin/sh

# Usage: sh sbuild-linux.sh [source directory] [build type] [config]
#
# The config names a file in etc/Config; "default" builds everything. Personal
# settings go in etc/Config/local.cmake, which is not tracked. For the number
# of build jobs, export CMAKE_BUILD_PARALLEL_LEVEL.

sh ${1:-DJV}/etc/Linux/sbuild.sh ${1:-DJV} ${2:-Release} ${3:-default}
