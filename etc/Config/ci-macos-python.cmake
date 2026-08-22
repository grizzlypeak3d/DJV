# Continuous integration on macOS, with the Python bindings. They are the
# reason for the second build of each platform: they bring shared libraries
# with them, and nothing else here builds either.
#
# Overrides come before the file they are based on: a plain cache set does not
# overwrite a value that is already there, so the first to set a value wins.
include("${CMAKE_CURRENT_LIST_DIR}/local.cmake" OPTIONAL)

# The top of the chain: default.cmake sets TLRENDER_PYTHON from this and
# ftk_PYTHON from that, so this covers all three sets of bindings --
# including DJV's own, which naming the middle of the chain left out.
set(DJV_PYTHON ON CACHE BOOL "")

include("${CMAKE_CURRENT_LIST_DIR}/ci-macos.cmake")
