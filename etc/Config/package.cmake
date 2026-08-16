# A package build: FFmpeg cut down to the codecs that can be shipped without
# a license, with both ways of reaching it. The plugin against that minimal
# build is enough for limited exports; the command line tool is how someone
# brings their own codecs. The commercial DJV Studio package is the other way
# round -- the full FFmpeg and the plugin -- and its own package config sets
# that.
include("${CMAKE_CURRENT_LIST_DIR}/local.cmake" OPTIONAL)

# FORCE, so that a personal local.cmake cannot change what a package ships:
# it is included first, and a plain cache set would lose to it. A -D on the
# command line still wins, since CMake reads those after this file.
set(TLRENDER_FFMPEG_MINIMAL ON CACHE BOOL "" FORCE)
set(TLRENDER_FFMPEG_CMD ON CACHE BOOL "" FORCE)
set(TLRENDER_FFMPEG_PLUGIN ON CACHE BOOL "" FORCE)
set(DJV_TESTS OFF CACHE BOOL "")

include("${CMAKE_CURRENT_LIST_DIR}/ci.cmake")
