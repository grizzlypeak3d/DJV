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
string(REGEX MATCH "VERSION_DEV \"([0-9a-z.-]*)\"" _ ${VERSION_H})
set(DJV_VERSION_DEV ${CMAKE_MATCH_1})
# Composed here rather than read: the header builds it from the macros
# above, so there is no literal in the file to match. Keep this in step
# with the way the header spells it.
set(DJV_VERSION_FULL
    "${DJV_VERSION_MAJOR}.${DJV_VERSION_MINOR}.${DJV_VERSION_PATCH}${DJV_VERSION_DEV}")

project(
    DJV
    VERSION ${DJV_VERSION_MAJOR}.${DJV_VERSION_MINOR}.${DJV_VERSION_PATCH}
    DESCRIPTION "DJV is an open source application for media playback and review."
    HOMEPAGE_URL "https://github.com/grizzlypeak3d/DJV"
    LANGUAGES CXX)

# A Python wheel build: scikit-build-core sets SKBUILD. The wheel settings
# are read before the options below, the way "cmake -C" would read them, so
# they win.
if(SKBUILD)
    include(etc/Config/wheel.cmake)
    include(etc/Python/djvWheel.cmake)
endif()

#-------------------------------------------------------------------------------
# Options

# Packaging gathers the dependencies out of the install prefix, where the
# super build put them; a build against system packages has none there.
# The package is made either way, with or without them in it.
set(DJV_PACKAGE_DEPS ON CACHE BOOL "Package the super build's dependencies with the install")
set(DJV_MACOS_PACKAGE OFF CACHE BOOL "Enable packaging for macOS")
set(DJV_WINDOWS_PACKAGE OFF CACHE BOOL "Enable packaging for Windows")
set(DJV_PROGRAMS ON CACHE BOOL "Build programs")
set(DJV_PYTHON OFF CACHE BOOL "Enable support for Python")
# Where the djv package goes, relative to the install prefix. The default
# keeps "PYTHONPATH=<prefix>/lib" working; a wheel puts the package at the
# top of its install.
set(DJV_PYTHON_INSTALL_DIR "lib/djv" CACHE STRING "Python package install directory")
set(DJV_TLRENDER_PACKAGE OFF CACHE BOOL "Use an installed tlRender rather than deps/tlRender")
set(DJV_TESTS OFF CACHE BOOL "Build tests")
set(DJV_TESTS_NO_GL OFF CACHE BOOL "Run only the tests that do not need OpenGL")
set(DJV_GCOV OFF CACHE BOOL "Enable gcov code coverage")
set(DJV_SANITIZE "" CACHE STRING "Enable sanitizers (comma-separated, e.g. address,undefined)")

#-------------------------------------------------------------------------------
# Internal configuration

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/Modules)

# The libraries mark what only development needs -- static libraries,
# headers, CMake configuration -- as "dev"; everything else lands here, and
# packaging takes this component alone. Installing to a prefix takes every
# component, so a source build still gets the development files.
set(CMAKE_INSTALL_DEFAULT_COMPONENT_NAME runtime)

if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 20)
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

if(DJV_GCOV)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage -fprofile-update=atomic")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --coverage -fprofile-update=atomic")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage -fprofile-update=atomic")
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
    # Only the bundle keeps the libraries in Frameworks; the zip and a
    # prefix install put them in lib, so the search path follows the
    # layout being made.
    if(DJV_MACOS_PACKAGE)
        set(CMAKE_INSTALL_RPATH "@executable_path/../Frameworks")
    else()
        set(CMAKE_INSTALL_RPATH "@executable_path/../lib")
    endif()
    # The freshly built libraries have to come before the installed
    # copies, or a binary run from the build tree quietly loads the
    # stale install -- so they are gathered into one directory that
    # leads the search path. The install prefix entry cannot simply be
    # dropped: CMake adds it back for the prebuilt dependencies, and
    # after the build tree's own directories only because they are
    # listed here first.
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_BUILD_RPATH
        "${CMAKE_BINARY_DIR}/lib"
        "${CMAKE_INSTALL_PREFIX}/lib")
else()
    set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib")
endif()
# The rest of an rpath, relative to <prefix>/lib: a wheel names the libraries
# of the tlrender and feather_tk packages beside it. The libraries are loaded
# by Python as well as by the executable there, so on macOS the search starts
# from the library rather than the executable.
if(DJV_INSTALL_RPATH_EXTRA)
    if(APPLE)
        set(CMAKE_INSTALL_RPATH "@loader_path/../lib")
        foreach(path ${DJV_INSTALL_RPATH_EXTRA})
            list(APPEND CMAKE_INSTALL_RPATH "@loader_path/${path}")
        endforeach()
    elseif(UNIX)
        foreach(path ${DJV_INSTALL_RPATH_EXTRA})
            list(APPEND CMAKE_INSTALL_RPATH "$ORIGIN/${path}")
        endforeach()
    endif()
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

#-------------------------------------------------------------------------------
# Dependencies

find_package(ZLIB REQUIRED)

# Python dependencies
#
# The djv module is built the way feather-tk's is, the stable ABI or not and
# against the same nanobind library, since nanobind shares types only
# between modules built alike and djv takes its types from feather_tk and
# tlrender. So this is feather-tk's own option, declared here too because
# feather-tk is not added until below.
set(ftk_PYTHON_STABLE_ABI ON CACHE BOOL "Build the Python module against the stable ABI")
if(DJV_PYTHON)
    # Development.Module rather than Development, as feather-tk asks: an
    # extension module does not link libpython.
    set(DJV_PYTHON_COMPONENTS Interpreter Development.Module)
    set(DJV_PYTHON_VERSION 3.10)
    set(DJV_NANOBIND_LIBRARY nanobind-static)
    if(ftk_PYTHON_STABLE_ABI)
        # nanobind supports the stable ABI from Python 3.12.
        list(APPEND DJV_PYTHON_COMPONENTS Development.SABIModule)
        set(DJV_PYTHON_VERSION 3.12)
        set(DJV_NANOBIND_LIBRARY nanobind-static-abi3)
    endif()
    find_package(Python ${DJV_PYTHON_VERSION} REQUIRED COMPONENTS ${DJV_PYTHON_COMPONENTS})
    find_package(nanobind REQUIRED CONFIG)
endif()

#-------------------------------------------------------------------------------
# tlRender

if(DJV_TLRENDER_PACKAGE)
    # tlRender's package finds feather-tk's.
    find_package(tlRender REQUIRED)
    # The resource tool is the feather-tk package's imported target; the
    # name ftk_icon_resources() falls back on is the one inside feather-tk's
    # own tree.
    set(ftk_RESOURCE_COMMAND ftk::ftk-resource)
    if(DJV_PYTHON)
        # feather-tk builds this library when it is added with tlRender; an
        # installed one leaves it to us. Built the same way, since the
        # binding libraries are compiled into the module against it.
        nanobind_build_library(${DJV_NANOBIND_LIBRARY})
        if(ftk_PYTHON_STABLE_ABI)
            target_compile_definitions(${DJV_NANOBIND_LIBRARY} PUBLIC Py_LIMITED_API=0x030C0000)
        endif()
    endif()
else()
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

    add_subdirectory(deps/tlRender)
    add_library(tlRender::tlCore ALIAS tlCore)
    add_library(tlRender::tlGL ALIAS tlGL)
    add_library(tlRender::tlIO ALIAS tlIO)
    add_library(tlRender::tlTimeline ALIAS tlTimeline)
    add_library(tlRender::tlUI ALIAS tlUI)
endif()

#-------------------------------------------------------------------------------
# Sub-directories

add_subdirectory(lib/djv)
if(DJV_PROGRAMS)
    add_subdirectory(bin)
endif()
if(DJV_TESTS)
    add_subdirectory(tests)
endif()
# Before the documentation, which installs the licenses page this writes.
add_subdirectory(etc/Legal)
add_subdirectory(docs)
# A wheel is the application and its documentation: the sample media and the
# desktop integration are for the packages.
if(NOT SKBUILD)
    add_subdirectory(etc/SampleData)
    if(UNIX AND NOT APPLE)
        add_subdirectory(etc/Linux)
    endif()
endif()

include(CMakePackageConfigHelpers)
set(INCLUDE_INSTALL_DIR include/djv)
configure_package_config_file(
    djvConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/djvConfig.cmake
    INSTALL_DESTINATION lib/cmake/djv
    PATH_VARS INCLUDE_INSTALL_DIR)
install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/djvConfig.cmake
    DESTINATION lib/cmake/djv
    COMPONENT dev)

#-------------------------------------------------------------------------------
# Packaging

# Always: the package target is how a build is turned into an archive, and
# a build against the system's packages wants one as much as a super build
# does -- it holds DJV alone. DJV_PACKAGE_DEPS decides whether the super
# build's libraries are gathered into it; see cmake/Modules/Package.cmake.
include(Package)
include(CPack)
include(InstallRequiredSystemLibraries)
