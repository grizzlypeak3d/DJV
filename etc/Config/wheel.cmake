# Python wheels: the djv package with the application, the libraries they
# load, and the documentation.
#
# Read by the main project when scikit-build-core runs it (see
# pyproject.toml), before the options, the way "cmake -C" would be. There is
# no local.cmake here: a wheel should build the same on every machine.

set(DJV_PYTHON ON CACHE BOOL "")
set(ftk_PYTHON_STABLE_ABI ON CACHE BOOL "")
set(DJV_PROGRAMS ON CACHE BOOL "")
set(DJV_TESTS OFF CACHE BOOL "")

# The packages gather the super build's libraries into the install; a wheel
# has none of its own, see below.
set(DJV_PACKAGE_DEPS OFF CACHE BOOL "")

# tlRender and feather-tk are their wheels, installed beside this one: their
# libraries are loaded from there rather than carried again, so that one
# process has one of each. The package is the wheel's install directory
# (wheel.install-dir), so the module goes at its top, the libraries in lib
# and the application in bin beside it, and all of them name the tlrender
# and feather_tk packages next door.
set(DJV_TLRENDER_PACKAGE ON CACHE BOOL "")
set(DJV_PYTHON_INSTALL_DIR "." CACHE STRING "")
set(DJV_INSTALL_RPATH_EXTRA "../../tlrender/lib;../../feather_tk/lib" CACHE STRING "")

# Shared, so that the application and the module share one copy of DJV.
set(BUILD_SHARED_LIBS ON CACHE BOOL "")

# Homebrew is /opt/homebrew on Apple silicon and /usr/local on Intel, and
# only the first is ignored by default; its zlib is not the one the
# feather-tk wheel carries.
if(APPLE)
    set(CMAKE_IGNORE_PREFIX_PATH "/opt/homebrew;/usr/local" CACHE STRING "")
endif()

# libGL rather than the GLVND libraries FindOpenGL prefers: libGL is among the
# libraries a manylinux wheel may take from the system, and libOpenGL is not.
set(OpenGL_GL_PREFERENCE LEGACY CACHE STRING "")
