# A macOS package, built back to the oldest supported system and for a
# named architecture rather than the one that happened to build it.
include("${CMAKE_CURRENT_LIST_DIR}/local.cmake" OPTIONAL)

set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "")
set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "")

include("${CMAKE_CURRENT_LIST_DIR}/package.cmake")
