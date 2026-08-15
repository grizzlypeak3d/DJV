# Build configuration: what to build, in one file for every platform.
#
# Used with "cmake -C", which reads it before the project, so everything here
# lands in the cache as a default. Plain CMake rather than shell, so the same
# file serves Linux, macOS, Windows and CI.
#
# Personal settings go in etc/Config/local.cmake, which is not tracked. It is
# included first, and a plain cache set does not overwrite a value already
# there, so anything it sets wins. The files beside this one layer the same
# way: their own values come before the file they are based on.
#
# For the number of build jobs, set CMAKE_BUILD_PARALLEL_LEVEL.
include("${CMAKE_CURRENT_LIST_DIR}/local.cmake" OPTIONAL)

set(DJV_TESTS OFF CACHE BOOL "")
set(TLRENDER_NET OFF CACHE BOOL "")
set(TLRENDER_OCIO ON CACHE BOOL "")
set(TLRENDER_JPEG ON CACHE BOOL "")
set(TLRENDER_TIFF ON CACHE BOOL "")
set(TLRENDER_EXR ON CACHE BOOL "")
# The assembly heavy codecs need NASM, which Windows has no toolchain for.
if(WIN32)
    set(TLRENDER_AOM OFF CACHE BOOL "")
    set(TLRENDER_SVTAV1 OFF CACHE BOOL "")
    set(TLRENDER_NASM OFF CACHE BOOL "")
else()
    set(TLRENDER_AOM ON CACHE BOOL "")
    set(TLRENDER_SVTAV1 ON CACHE BOOL "")
    set(TLRENDER_NASM ON CACHE BOOL "")
endif()
set(TLRENDER_FFMPEG ON CACHE BOOL "")
set(TLRENDER_FFMPEG_MINIMAL OFF CACHE BOOL "")
set(TLRENDER_FFMPEG_PLUGIN ON CACHE BOOL "")
set(TLRENDER_FFMPEG_CMD OFF CACHE BOOL "")
set(TLRENDER_OIIO ON CACHE BOOL "")
set(TLRENDER_USD OFF CACHE BOOL "")
set(TLRENDER_PYTHON OFF CACHE BOOL "")
set(TLRENDER_PROGRAMS OFF CACHE BOOL "")
set(TLRENDER_EXAMPLES OFF CACHE BOOL "")
set(TLRENDER_TESTS OFF CACHE BOOL "")
set(ftk_API "GL_4_1" CACHE STRING "")
set(ftk_EXAMPLES OFF CACHE BOOL "")
set(ftk_TESTS OFF CACHE BOOL "")
set(BUILD_SHARED_LIBS OFF CACHE BOOL "")

if(APPLE)
    # The deployment target is policy: the oldest system supported. The
    # architecture is not -- it is whatever is doing the building, and naming
    # one here cross compiles anywhere else. Packages name it; builds discover
    # it.
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "")
endif()
