# The default build against SDL2, which stays supported while SDL3 is the default.
include("${CMAKE_CURRENT_LIST_DIR}/local.cmake" OPTIONAL)

set(ftk_SDL2 ON CACHE BOOL "Build SDL2")
set(ftk_SDL3 OFF CACHE BOOL "Build SDL3")

include("${CMAKE_CURRENT_LIST_DIR}/default.cmake")
