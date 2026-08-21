# What continuous integration builds everywhere: the default configuration,
# which is no Python bindings and static libraries. The -python
# configurations beside this one are the other half, and each platform is
# built both ways.
include("${CMAKE_CURRENT_LIST_DIR}/local.cmake" OPTIONAL)

include("${CMAKE_CURRENT_LIST_DIR}/default.cmake")
