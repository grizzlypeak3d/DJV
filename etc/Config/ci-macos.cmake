# Continuous integration on macOS, which targets a newer system than
# the package does.
include("${CMAKE_CURRENT_LIST_DIR}/local.cmake" OPTIONAL)

set(CMAKE_OSX_DEPLOYMENT_TARGET "14" CACHE STRING "")

# djv-test initializes the timeline rather than the UI and makes no OpenGL
# context, so it runs here as well as on Linux. Without this the job's test
# step finds nothing to run and passes for it. The Python tests that need
# OpenGL read the flag and skip themselves.
set(DJV_TESTS ON CACHE BOOL "")
set(DJV_TESTS_NO_GL ON CACHE BOOL "")

include("${CMAKE_CURRENT_LIST_DIR}/ci.cmake")
