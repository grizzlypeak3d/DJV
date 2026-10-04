# The macOS package for Intel Macs, made on an Apple silicon Mac. A package
# of its own rather than one universal one: the dependencies with their own
# build systems would each have to be built twice and merged, and Intel Macs
# are on the way out, so this is the one to drop.
#
# Build it in a directory of its own, since the build directories are named
# for the build type and not the architecture:
#
#     mkdir x64 && cd x64
#     sh ../DJV/package-macos.sh ../DJV Release x64
set(DJV_MACOS_PACKAGE ON CACHE BOOL "" FORCE)

set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "")
set(CMAKE_OSX_ARCHITECTURES "x86_64" CACHE STRING "")

include("${CMAKE_CURRENT_LIST_DIR}/package.cmake")
