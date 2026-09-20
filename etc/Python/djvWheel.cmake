# A wheel build is against the tlRender and feather-tk wheels, which are
# build requirements: the CMake packages in them are found where pip installed
# them, and the libraries are named by rpath from the packages beside this
# one once installed (see etc/Config/wheel.cmake). There is nothing else to
# build first.

execute_process(
    COMMAND ${Python_EXECUTABLE} -c
        "import importlib.util, os; print(';'.join(os.path.dirname(importlib.util.find_spec(n).origin) for n in ('tlrender', 'feather_tk')))"
    OUTPUT_VARIABLE DJV_WHEEL_PACKAGES
    OUTPUT_STRIP_TRAILING_WHITESPACE
    COMMAND_ERROR_IS_FATAL ANY)
list(GET DJV_WHEEL_PACKAGES 0 DJV_WHEEL_TLRENDER)
list(GET DJV_WHEEL_PACKAGES 1 DJV_WHEEL_FTK)
file(TO_CMAKE_PATH "${DJV_WHEEL_TLRENDER}" DJV_WHEEL_TLRENDER)
file(TO_CMAKE_PATH "${DJV_WHEEL_FTK}" DJV_WHEEL_FTK)
message(STATUS "Using tlRender from ${DJV_WHEEL_TLRENDER}")
message(STATUS "Using feather-tk from ${DJV_WHEEL_FTK}")

# feather-tk as well, since zlib is found before tlRender's package is, and
# the feather-tk wheel's is the one to link.
list(PREPEND CMAKE_PREFIX_PATH ${DJV_WHEEL_TLRENDER} ${DJV_WHEEL_FTK})
