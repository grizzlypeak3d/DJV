# Continuous integration on Linux against OpenGL ES 3, without the media
# libraries: enough to catch code that will not compile there.
include("${CMAKE_CURRENT_LIST_DIR}/local.cmake" OPTIONAL)

set(ftk_API "GLES_3" CACHE STRING "")
set(DJV_TESTS ON CACHE BOOL "")
set(TLRENDER_AOM OFF CACHE BOOL "")
set(TLRENDER_SVTAV1 OFF CACHE BOOL "")
set(TLRENDER_FFMPEG OFF CACHE BOOL "")
set(TLRENDER_FFMPEG_PLUGIN OFF CACHE BOOL "")
set(TLRENDER_OCIO OFF CACHE BOOL "")
set(TLRENDER_OIIO OFF CACHE BOOL "")

include("${CMAKE_CURRENT_LIST_DIR}/ci.cmake")
