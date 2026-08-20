# What continuous integration builds everywhere: the default
# configuration, plus the Python bindings, which nothing else builds.
include("${CMAKE_CURRENT_LIST_DIR}/local.cmake" OPTIONAL)

set(TLRENDER_PYTHON ON CACHE BOOL "")

include("${CMAKE_CURRENT_LIST_DIR}/default.cmake")
