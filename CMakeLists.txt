cmake_minimum_required(VERSION 3.31)
cmake_policy(SET CMP0048 NEW)
cmake_policy(SET CMP0091 NEW)

file(READ "lib/djv/Models/Version.h" VERSION_H)
string(REGEX MATCH "VERSION_MAJOR ([0-9]*)" _ ${VERSION_H})
set(DJV_VERSION_MAJOR ${CMAKE_MATCH_1})
string(REGEX MATCH "VERSION_MINOR ([0-9]*)" _ ${VERSION_H})
set(DJV_VERSION_MINOR ${CMAKE_MATCH_1})
string(REGEX MATCH "VERSION_PATCH ([0-9]*)" _ ${VERSION_H})
set(DJV_VERSION_PATCH ${CMAKE_MATCH_1})
string(REGEX MATCH "VERSION_DEV \"([0-9a-z\.\-]*)\"" _ ${VERSION_H})
set(DJV_VERSION_DEV ${CMAKE_MATCH_1})
string(REGEX MATCH "VERSION_FULL \"([0-9a-z\.\-]*)\"" _ ${VERSION_H})
set(DJV_VERSION_FULL ${CMAKE_MATCH_1})

project(
    DJV
    VERSION ${DJV_VERSION_MAJOR}.${DJV_VERSION_MINOR}.${DJV_VERSION_PATCH}
    DESCRIPTION "DJV is an open source application for media playback and review."
    HOMEPAGE_URL "https://github.com/grizzlypeak3d/DJV"
    LANGUAGES CXX)

#-------------------------------------------------------------------------------
# Options

set(DJV_MACOS_PACKAGE OFF CACHE BOOL "Enable packaging for macOS")
set(DJV_WINDOWS_PACKAGE OFF CACHE BOOL "Enable packaging for Windows")
set(DJV_PROGRAMS ON CACHE BOOL "Build programs")
set(DJV_TESTS OFF CACHE BOOL "Build tests")
set(DJV_SANITIZE "" CACHE STRING "Enable sanitizers (comma-separated, e.g. address,undefined)")

#-------------------------------------------------------------------------------
# Internal configuration

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/Modules)

if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
endif()

if(DJV_TESTS)
    set(CTEST_OUTPUT_ON_FAILURE ON)
    enable_testing()
endif()

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreaded$<$<CONFIG:Debug>:Debug>DLL)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter")
endif()

if(DJV_SANITIZE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=${DJV_SANITIZE} -fno-omit-frame-pointer")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=${DJV_SANITIZE} -fno-omit-frame-pointer")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=${DJV_SANITIZE}")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=${DJV_SANITIZE}")
endif()

if(APPLE)
    set(CMAKE_FIND_FRAMEWORK LAST)
endif()

if(WIN32)
elseif(APPLE)
    set(CMAKE_INSTALL_RPATH "@executable_path/../Frameworks")
    set(CMAKE_BUILD_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
else()
    set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib")
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

#-------------------------------------------------------------------------------
# Dependencies

find_package(ZLIB REQUIRED)

#-------------------------------------------------------------------------------
# Git submodules

find_package(Git REQUIRED)
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/deps/tlRender/CMakeLists.txt")
    execute_process(
        COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        RESULT_VARIABLE GIT_SUBMODULE_RESULT)
    if(NOT GIT_SUBMODULE_RESULT EQUAL 0)
        message(FATAL_ERROR "git submodule update failed with ${GIT_SUBMODULE_RESULT}")
    endif()
endif()

#-------------------------------------------------------------------------------
# Sub-directories

add_subdirectory(deps/tlRender)
add_library(tlRender::tlCore ALIAS tlCore)
add_library(tlRender::tlGL ALIAS tlGL)
add_library(tlRender::tlIO ALIAS tlIO)
add_library(tlRender::tlDevice ALIAS tlDevice)
add_library(tlRender::tlTimeline ALIAS tlTimeline)
add_library(tlRender::tlUI ALIAS tlUI)

add_subdirectory(lib/djv)
if(DJV_PROGRAMS)
    add_subdirectory(bin)
endif()
if(DJV_TESTS)
    add_subdirectory(tests)
endif()
add_subdirectory(etc/Legal)
add_subdirectory(etc/SampleData)

include(CMakePackageConfigHelpers)
set(INCLUDE_INSTALL_DIR include/djv)
configure_package_config_file(
    djvConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/djvConfig.cmake
    INSTALL_DESTINATION lib/cmake/djv
    PATH_VARS INCLUDE_INSTALL_DIR)
install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/djvConfig.cmake
    DESTINATION lib/cmake/djv)

#-------------------------------------------------------------------------------
# Packaging

include(Package)
include(CPack)
include(InstallRequiredSystemLibraries)
