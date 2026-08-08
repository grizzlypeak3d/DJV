# Continuous integration on macOS, which targets a newer system than
# the package does.
include("${CMAKE_CURRENT_LIST_DIR}/local.cmake" OPTIONAL)

set(CMAKE_OSX_DEPLOYMENT_TARGET "14" CACHE STRING "")

include("${CMAKE_CURRENT_LIST_DIR}/ci.cmake")
