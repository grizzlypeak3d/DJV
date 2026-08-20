# Continuous integration on Windows.
include("${CMAKE_CURRENT_LIST_DIR}/local.cmake" OPTIONAL)

# djv-test initializes the timeline rather than the UI and makes no OpenGL
# context, so it runs here as well as on Linux. Without this the job's test
# step finds nothing to run and passes for it. The Python tests that need
# OpenGL read the flag and skip themselves.
set(DJV_TESTS ON CACHE BOOL "")
set(DJV_TESTS_NO_GL ON CACHE BOOL "")

include("${CMAKE_CURRENT_LIST_DIR}/ci.cmake")
