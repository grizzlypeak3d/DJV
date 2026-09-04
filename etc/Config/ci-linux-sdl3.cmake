# Continuous integration on Linux against SDL3 rather than SDL2.
include("${CMAKE_CURRENT_LIST_DIR}/local.cmake" OPTIONAL)

set(ftk_SDL2 OFF CACHE BOOL "Build SDL2")
set(ftk_SDL3 ON CACHE BOOL "Build SDL3")

include("${CMAKE_CURRENT_LIST_DIR}/ci-linux.cmake")
