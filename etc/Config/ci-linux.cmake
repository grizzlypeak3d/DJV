# Continuous integration on Linux, which runs the tests.
include("${CMAKE_CURRENT_LIST_DIR}/local.cmake" OPTIONAL)

set(DJV_TESTS ON CACHE BOOL "")

include("${CMAKE_CURRENT_LIST_DIR}/ci.cmake")
