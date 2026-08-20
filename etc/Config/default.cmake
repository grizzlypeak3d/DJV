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
# Say so: the file is not tracked, so a build that behaves oddly has nothing
# else to notice it by. Every config reaches this file, so once here covers
# all of them.
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/local.cmake")
    message(STATUS "etc/Config/local.cmake is in use; personal settings are affecting this build")
endif()

set(DJV_TESTS OFF CACHE BOOL "")
set(TLRENDER_NET OFF CACHE BOOL "")
set(TLRENDER_OCIO ON CACHE BOOL "")
set(TLRENDER_JPEG ON CACHE BOOL "")
set(TLRENDER_TIFF ON CACHE BOOL "")
set(TLRENDER_EXR ON CACHE BOOL "")
# The assembly heavy codecs need NASM, and the superbuild builds it with
# autotools, which needs a Unix shell. Turning these on for Windows means
# installing NASM there and letting the codecs find it on PATH.
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
set(TLRENDER_OIIO ON CACHE BOOL "")
set(TLRENDER_USD OFF CACHE BOOL "")
set(TLRENDER_PYTHON OFF CACHE BOOL "")
# One switch for the whole stack: feather-tk and DJV have their own Python
# options, and a value already in the cache wins, so setting TLRENDER_PYTHON
# first is enough to turn all three on.
set(ftk_PYTHON ${TLRENDER_PYTHON} CACHE BOOL "")
set(DJV_PYTHON ${TLRENDER_PYTHON} CACHE BOOL "")
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
