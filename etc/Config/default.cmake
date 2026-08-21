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
# The assembly heavy codecs need NASM. It is built from source on the
# platforms that can and installed beforehand on Windows, where the README
# asks for it already, so the codecs find it on PATH either way.
set(TLRENDER_AOM ON CACHE BOOL "")
set(TLRENDER_SVTAV1 ON CACHE BOOL "")
if(WIN32)
    set(TLRENDER_NASM OFF CACHE BOOL "")
else()
    set(TLRENDER_NASM ON CACHE BOOL "")
endif()
set(TLRENDER_FFMPEG ON CACHE BOOL "")
set(TLRENDER_FFMPEG_MINIMAL OFF CACHE BOOL "")
set(TLRENDER_FFMPEG_PLUGIN ON CACHE BOOL "")
set(TLRENDER_OIIO ON CACHE BOOL "")
set(TLRENDER_USD OFF CACHE BOOL "")
# The Python bindings, derived down the stack the way the dependencies run:
# djvPy imports tlRenderPy, which needs ftkPy. Setting the outermost one is
# enough, and because local.cmake is read first, any level can still be named
# on its own to turn that one on without the ones above it.
set(DJV_PYTHON OFF CACHE BOOL "")
set(TLRENDER_PYTHON ${DJV_PYTHON} CACHE BOOL "")
set(ftk_PYTHON ${TLRENDER_PYTHON} CACHE BOOL "")
set(TLRENDER_PROGRAMS OFF CACHE BOOL "")
set(TLRENDER_EXAMPLES OFF CACHE BOOL "")
set(TLRENDER_TESTS OFF CACHE BOOL "")
set(ftk_API "GL_4_1" CACHE STRING "")
set(ftk_EXAMPLES OFF CACHE BOOL "")
set(ftk_TESTS OFF CACHE BOOL "")
# Shared when Python is on: each binding module would otherwise carry its
# own static copy of the stack, and three copies of SDL in one process
# fight over the same Objective-C classes -- window creation through the
# losing copy gets a legacy OpenGL context.
#
# Not on Windows. The libraries do carry export macros now -- FTK_API,
# TL_API and DJV_API, selected by the *_EXPORTS and *_STATIC definitions this
# flag sets -- so that is no longer what stands in the way. What does is that
# *_EXPORTS is defined PUBLIC, so a project consuming these libraries compiles
# their API as dllexport where it should be dllimport. Windows can be shared
# once that is sorted.
if(WIN32)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "")
else()
    set(BUILD_SHARED_LIBS ${TLRENDER_PYTHON} CACHE BOOL "")
endif()

if(APPLE)
    # The deployment target is policy: the oldest system supported. The
    # architecture is not -- it is whatever is doing the building, and naming
    # one here cross compiles anywhere else. Packages name it; builds discover
    # it.
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "")
endif()
