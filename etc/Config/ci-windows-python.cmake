# Continuous integration on Windows, with the Python bindings. They are the
# reason for the second build of each platform: they bring shared libraries
# with them, and nothing else here builds either.
#
# Overrides come before the file they are based on: a plain cache set does not
# overwrite a value that is already there, so the first to set a value wins.
include("${CMAKE_CURRENT_LIST_DIR}/local.cmake" OPTIONAL)

set(TLRENDER_PYTHON ON CACHE BOOL "")

include("${CMAKE_CURRENT_LIST_DIR}/ci-windows.cmake")
