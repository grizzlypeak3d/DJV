# Continuous integration on Windows, which has no NASM toolchain, so
# the assembly heavy codecs are left out.
include("${CMAKE_CURRENT_LIST_DIR}/local.cmake" OPTIONAL)

set(TLRENDER_AOM OFF CACHE BOOL "")
set(TLRENDER_SVTAV1 OFF CACHE BOOL "")
set(TLRENDER_NASM OFF CACHE BOOL "")

include("${CMAKE_CURRENT_LIST_DIR}/ci.cmake")
