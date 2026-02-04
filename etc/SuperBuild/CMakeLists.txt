cmake_minimum_required(VERSION 3.31)
cmake_policy(SET CMP0091 NEW)
cmake_policy(SET CMP0097 NEW)

project(DJV-SuperBuild)

#-------------------------------------------------------------------------------
# Build options

if(APPLE)
    set(DJV_IGNORE_PREFIX_PATH_DEFAULT /opt/homebrew)
endif()
set(DJV_IGNORE_PREFIX_PATH ${DJV_IGNORE_PREFIX_PATH_DEFAULT} CACHE STRING "Ignore the given prefix path")

set(ftk_API "GL_4_1" CACHE STRING "Graphics API (GL_4_1, GL_4_1_Debug, GLES_2)")

set(TLRENDER_NET OFF CACHE BOOL "Enable network support")
set(TLRENDER_OCIO ON CACHE BOOL "Enable support for OpenColorIO")
set(TLRENDER_JPEG ON CACHE BOOL "Enable support for JPEG")
set(TLRENDER_TIFF ON CACHE BOOL "Enable support for TIFF")
set(TLRENDER_EXR ON CACHE BOOL "Enable support for OpenEXR")
set(TLRENDER_FFMPEG ON CACHE BOOL "Enable support for FFmpeg (see the README for building FFmpeg on Windows)")
set(TLRENDER_FFMPEG_MINIMAL OFF CACHE BOOL "Enable a minimal set of FFmpeg codecs")
set(TLRENDER_NASM_DEFAULT (TLRENDER_NET OR TLRENDER_AV1 OR TLRENDER_JPEG OR TLRENDER_FFMPEG) AND NOT WIN32 AND NOT BSD)
set(TLRENDER_NASM ${TLRENDER_NASM_DEFAULT} CACHE BOOL "Enable building NASM")
set(TLRENDER_OIIO ON CACHE BOOL "Enable support for OpenImageIO")
set(TLRENDER_USD OFF CACHE BOOL "Enable support for USD")
set(TLRENDER_USD_PYTHON python3 CACHE STRING "Python executable for building USD")
set(TLRENDER_BMD OFF CACHE BOOL "Enable support for Blackmagic Design devices")
set(TLRENDER_BMD_SDK "" CACHE PATH "Full path to the Blackmagic Design SDK")

#-------------------------------------------------------------------------------
# Internal configuration

# \bug Add the OpenColorIO cmake modules.
list(PREPEND CMAKE_MODULE_PATH
    ${PROJECT_SOURCE_DIR}/../../cmake/Modules
    ${PROJECT_SOURCE_DIR}/cmake/Modules
    ${CMAKE_INSTALL_PREFIX}/share/OpenColorIO/cmake/modules)

if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
endif()

if(NOT BUILD_SHARED_LIBS)
    set(BUILD_SHARED_LIBS OFF)
endif()

if(DJV_IGNORE_PREFIX_PATH)
    set(CMAKE_IGNORE_PREFIX_PATH ${DJV_IGNORE_PREFIX_PATH})
endif()

if(APPLE)
    set(CMAKE_FIND_FRAMEWORK LAST)
endif()

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreaded$<$<CONFIG:Debug>:Debug>DLL)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP")
endif()

# https://stackoverflow.com/questions/45414507/pass-a-list-of-prefix-paths-to-externalproject-add-in-cmake-args
string(REPLACE ";" "|" CMAKE_MODULE_PATH_TMP "${CMAKE_MODULE_PATH}")
string(REPLACE ";" "|" CMAKE_PREFIX_PATH_TMP "${CMAKE_PREFIX_PATH}")

# Common arguments for building external projects.
set(DJV_EXTERNAL_ARGS
    -DCMAKE_MODULE_PATH=${CMAKE_MODULE_PATH_TMP}
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
    -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH_TMP}
    -DCMAKE_IGNORE_PREFIX_PATH=${CMAKE_IGNORE_PREFIX_PATH}
    -DCMAKE_POLICY_DEFAULT_CMP0091:STRING=NEW
    -DCMAKE_POLICY_DEFAULT_CMP0114:STRING=NEW
    -DCMAKE_FIND_FRAMEWORK=${CMAKE_FIND_FRAMEWORK}
    -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
    -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
    -DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}
    -DCMAKE_MSVC_RUNTIME_LIBRARY=${CMAKE_MSVC_RUNTIME_LIBRARY}
    -DCMAKE_VERBOSE_MAKEFILE=${CMAKE_VERBOSE_MAKEFILE}
    -DCMAKE_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET}
    -DCMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}
    -DBUILD_SHARED_LIBS=${BUILD_SHARED_LIBS})
string(REPLACE ";" " " DJV_EXTERNAL_ARGS_TMP "${DJV_EXTERNAL_ARGS}")
message("DJV_EXTERNAL_ARGS: " ${DJV_EXTERNAL_ARGS_TMP})

include(BuildtlRender)
