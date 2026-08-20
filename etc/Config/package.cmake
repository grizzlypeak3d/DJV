# A package build: FFmpeg cut down to the codecs that can be shipped without
# a license, with both ways of reaching it. The plugin against that minimal
# build is enough for limited exports; the command line tool is how someone
# brings their own codecs. The commercial DJV Studio package is the other way
# round -- the full FFmpeg and the plugin -- and its own package config sets
# that.
# A package build takes no personal settings. local.cmake is where the tests,
# examples, programs and Python bindings are turned on, and none of those
# belong in what someone installs.
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/local.cmake")
    message(FATAL_ERROR
        "etc/Config/local.cmake is present; move it aside for a package build")
endif()

# FORCE, so that nothing included below can change what a package ships. A -D
# on the command line still wins, since CMake reads those after this file.
set(TLRENDER_FFMPEG_MINIMAL ON CACHE BOOL "" FORCE)
set(TLRENDER_FFMPEG_PLUGIN ON CACHE BOOL "" FORCE)
set(DJV_TESTS OFF CACHE BOOL "")

# The defaults rather than ci.cmake. The two are the same today, but a
# package should not follow whatever continuous integration turns on next.
include("${CMAKE_CURRENT_LIST_DIR}/default.cmake")
