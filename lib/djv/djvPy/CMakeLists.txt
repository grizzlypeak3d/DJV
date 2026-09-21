# The module is private to the djv package, whose __init__.py gathers its
# API; the build tree lays the package out the way an install does, so
# PYTHONPATH can name either.
set(DJV_PYTHON_PACKAGE_DIR ${CMAKE_BINARY_DIR}/python/djv)

set(STABLE_ABI)
if(ftk_PYTHON_STABLE_ABI)
    set(STABLE_ABI STABLE_ABI)
endif()
nanobind_add_module(_djv ${STABLE_ABI} NB_STATIC Bindings.cpp)

target_include_directories(_djv PRIVATE ${PROJECT_SOURCE_DIR}/lib)

# The binding objects go into the module directly rather than through
# their archives. nanobind's own static library is first on the link line,
# and GNU ld takes from an archive only what is wanted at the moment it is
# scanned: a binding archive scanned after it finds the enum and datetime
# helpers already passed over, and a Release build fails to link.
target_sources(_djv PRIVATE $<TARGET_OBJECTS:djvModelsPy> $<TARGET_OBJECTS:djvUIPy>)
target_link_libraries(_djv PRIVATE djvModelsPy djvUIPy)

target_compile_options(_djv PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/bigobj>)

# The generator expression keeps multi-config generators from adding a
# directory per configuration, which would leave the module outside the
# package.
set_target_properties(_djv PROPERTIES
    FOLDER lib
    LIBRARY_OUTPUT_DIRECTORY $<1:${DJV_PYTHON_PACKAGE_DIR}>)
configure_file(__init__.py ${DJV_PYTHON_PACKAGE_DIR}/__init__.py COPYONLY)
configure_file(__main__.py ${DJV_PYTHON_PACKAGE_DIR}/__main__.py COPYONLY)

# The rpath convention of the executables names the interpreter's
# directory here, since the "executable" loading a Python module is
# Python itself: the module finds the libraries from where it is instead.
# That is <prefix>/lib from <prefix>/lib/djv, and lib inside the package in
# a wheel, where tlRender's and feather-tk's libraries are in the packages
# beside it (DJV_INSTALL_RPATH_EXTRA, relative to lib).
file(RELATIVE_PATH DJV_PYTHON_LIB_RPATH
    /prefix/${DJV_PYTHON_INSTALL_DIR} /prefix/lib)
set(RPATH_ORIGIN)
if(APPLE)
    set(RPATH_ORIGIN "@loader_path")
elseif(UNIX)
    set(RPATH_ORIGIN "$ORIGIN")
endif()
if(RPATH_ORIGIN)
    set(RPATHS "${RPATH_ORIGIN}/${DJV_PYTHON_LIB_RPATH}")
    foreach(path ${DJV_INSTALL_RPATH_EXTRA})
        file(RELATIVE_PATH path_from_module
            /prefix/${DJV_PYTHON_INSTALL_DIR} /prefix/lib/${path})
        list(APPEND RPATHS "${RPATH_ORIGIN}/${path_from_module}")
    endforeach()
    set_target_properties(_djv PROPERTIES INSTALL_RPATH "${RPATHS}")
endif()

# Type stubs: the signatures of a compiled module in a form editors and
# type checkers can read, since there is no Python source in it for them
# to parse. Written by nanobind from the bindings themselves, which imports
# the module that was just built -- through etc/Python/make_stub.py, which
# names the directories its libraries are in first, since on Windows they
# are not beside it and only os.add_dll_directory() finds them. Recursive,
# for the models and ui submodules: the stubs are a _djv directory beside
# the module. py.typed is what says they are to be trusted.
if(EXISTS ${NB_DIR}/src/stubgen.py)
    set(DJV_STUBGEN ${NB_DIR}/src/stubgen.py)
else()
    set(DJV_STUBGEN ${NB_DIR}/stubgen.py)
endif()
set(DJV_PYTHON_STUB ${DJV_PYTHON_PACKAGE_DIR}/_djv/__init__.pyi)
add_custom_command(
    OUTPUT ${DJV_PYTHON_STUB}
    COMMAND ${CMAKE_COMMAND} -E rm -rf ${DJV_PYTHON_PACKAGE_DIR}/_djv
    COMMAND ${Python_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/etc/Python/make_stub.py
        --dll-dir $<TARGET_FILE_DIR:djvModels>
        --dll-dir $<TARGET_FILE_DIR:djvUI>
        --dll-dir $<TARGET_FILE_DIR:djvApp>
        # tlRender's and feather-tk's libraries too: built here as
        # sub-projects, or in their packages in a wheel build. The
        # namespaced names are both.
        --dll-dir $<TARGET_FILE_DIR:tlRender::tlCore>
        --dll-dir $<TARGET_FILE_DIR:tlRender::tlGL>
        --dll-dir $<TARGET_FILE_DIR:tlRender::tlIO>
        --dll-dir $<TARGET_FILE_DIR:tlRender::tlTimeline>
        --dll-dir $<TARGET_FILE_DIR:tlRender::tlUI>
        --dll-dir $<TARGET_FILE_DIR:ftk::ftkCore>
        --dll-dir $<TARGET_FILE_DIR:ftk::ftkGL>
        --dll-dir $<TARGET_FILE_DIR:ftk::ftkUI>
        --dll-dir ${CMAKE_INSTALL_PREFIX}/bin
        --
        ${DJV_STUBGEN}
        -q
        -r
        -m djv._djv
        -i ${CMAKE_BINARY_DIR}/python
        # OpenTimelineIO, which the module imports: the super build
        # installs its Python package here, and a wheel build has it
        # in the build environment instead.
        -i ${CMAKE_INSTALL_PREFIX}/python
        -O ${DJV_PYTHON_PACKAGE_DIR}
    # The modules this one imports, where they are built here rather
    # than installed: they have to exist by the time it is imported.
    DEPENDS
        _djv
        $<TARGET_NAME_IF_EXISTS:_tlrender>
        $<TARGET_NAME_IF_EXISTS:_feather_tk>
        ${DJV_STUBGEN}
        ${PROJECT_SOURCE_DIR}/etc/Python/make_stub.py
    COMMENT "Generating the djv type stubs")
add_custom_target(_djv_stub ALL DEPENDS ${DJV_PYTHON_STUB})
file(TOUCH ${DJV_PYTHON_PACKAGE_DIR}/py.typed)

# What the stubs could not resolve is a C++ type the bindings never
# exposed; the build says so rather than leaving it in the files to be
# found. See etc/Python/check_stub.py.
add_custom_command(
    TARGET _djv_stub POST_BUILD
    COMMAND ${Python_EXECUTABLE}
        ${PROJECT_SOURCE_DIR}/etc/Python/check_stub.py
        ${DJV_PYTHON_PACKAGE_DIR}/_djv/__init__.pyi
        ${DJV_PYTHON_PACKAGE_DIR}/_djv/models.pyi
        ${DJV_PYTHON_PACKAGE_DIR}/_djv/ui.pyi)

install(TARGETS _djv
    LIBRARY DESTINATION ${DJV_PYTHON_INSTALL_DIR}
    RUNTIME DESTINATION ${DJV_PYTHON_INSTALL_DIR})
install(
    FILES
        __init__.py
        __main__.py
        ${DJV_PYTHON_PACKAGE_DIR}/py.typed
    DESTINATION ${DJV_PYTHON_INSTALL_DIR})
install(
    DIRECTORY ${DJV_PYTHON_PACKAGE_DIR}/_djv
    DESTINATION ${DJV_PYTHON_INSTALL_DIR})

# The rest is for a source build: a wheel runs the C++ application with the
# djv command, and its bindings are imported from where pip put them.
if(SKBUILD)
    return()
endif()

# A launcher that knows where the modules and the interpreter are, so
# running the Python application is one command rather than an
# environment to remember. The build tree names the module directories
# directly; the installed copy finds everything beside itself.
if(WIN32)
    # Two files rather than one: since Python 3.8 an extension module's
    # dependencies are not found through PATH on Windows, only through
    # os.add_dll_directory(), so a batch file that sets an environment and
    # calls main.py is not enough. The batch file is the name on PATH and the
    # bootstrap beside it is the part that has to be Python. Same reason
    # tests/run-python-tests.py exists.
    #
    # Under a per-configuration directory because the module paths below
    # differ between Release and Debug, and one generated file cannot hold
    # both. That is also where the Visual Studio generator puts every other
    # binary, so the launcher sits with them.
    #
    # The libraries come from the install prefix, not the build tree: there is
    # no rpath on Windows, and the built DLLs are spread over one directory per
    # target with nothing to tie them together. The Python tests resolve it the
    # same way, and sbuild installs at the end of every build. Run the install
    # target before the launcher if the libraries have changed.
    file(GENERATE
        OUTPUT ${CMAKE_BINARY_DIR}/bin/$<CONFIG>/djv-python-bootstrap.py
        CONTENT "# Generated by CMake; see lib/djv/djvPy/CMakeLists.txt.
import os
import runpy
import sys

for directory in [
        r\"${CMAKE_INSTALL_PREFIX}/bin\",
        r\"${CMAKE_INSTALL_PREFIX}/python/opentimelineio\"]:
    if os.path.isdir(directory):
        os.add_dll_directory(directory)

app = r\"${PROJECT_SOURCE_DIR}/examples/python\"
sys.path[0:0] = [
    r\"$<TARGET_FILE_DIR:_feather_tk>/..\",
    r\"$<TARGET_FILE_DIR:_tlrender>/..\",
    r\"$<TARGET_FILE_DIR:_djv>/..\",
    r\"${CMAKE_INSTALL_PREFIX}/python\",
    app]

runpy.run_path(os.path.join(app, \"main.py\"), run_name=\"__main__\")
")
    file(GENERATE
        OUTPUT ${CMAKE_BINARY_DIR}/bin/$<CONFIG>/djv-python.cmd
        CONTENT "@echo off
\"${Python_EXECUTABLE}\" \"%~dp0djv-python-bootstrap.py\" %*
")

    file(GENERATE
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/djv-python-bootstrap-install.py
        CONTENT "# Generated by CMake; see lib/djv/djvPy/CMakeLists.txt.
import os
import runpy
import sys

bindir = os.path.dirname(os.path.abspath(__file__))
prefix = os.path.dirname(bindir)

for directory in [bindir, os.path.join(prefix, \"python\", \"opentimelineio\")]:
    if os.path.isdir(directory):
        os.add_dll_directory(directory)

app = os.path.join(prefix, \"examples\", \"python\")
sys.path[0:0] = [
    os.path.join(prefix, \"lib\"),
    os.path.join(prefix, \"python\"),
    app]

runpy.run_path(os.path.join(app, \"main.py\"), run_name=\"__main__\")
")
    file(GENERATE
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/djv-python-install.cmd
        CONTENT "@echo off
\"${Python_EXECUTABLE}\" \"%~dp0djv-python-bootstrap.py\" %*
")
    install(
        PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/djv-python-install.cmd
        DESTINATION bin
        RENAME djv-python.cmd)
    install(
        PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/djv-python-bootstrap-install.py
        DESTINATION bin
        RENAME djv-python-bootstrap.py)
else()
    file(GENERATE
        OUTPUT ${CMAKE_BINARY_DIR}/bin/djv-python
        CONTENT "#!/bin/sh
PYTHONPATH=\"$<TARGET_FILE_DIR:_feather_tk>/..:$<TARGET_FILE_DIR:_tlrender>/..:$<TARGET_FILE_DIR:_djv>/..:${CMAKE_INSTALL_PREFIX}/python:$PYTHONPATH\" \\
exec \"${Python_EXECUTABLE}\" \"${PROJECT_SOURCE_DIR}/examples/python/main.py\" \"$@\"
"
        FILE_PERMISSIONS
            OWNER_READ OWNER_WRITE OWNER_EXECUTE
            GROUP_READ GROUP_EXECUTE
            WORLD_READ WORLD_EXECUTE)
    file(GENERATE
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/djv-python-install
        CONTENT "#!/bin/sh
dir=$(CDPATH= cd -- \"$(dirname -- \"$0\")\" && pwd)
PYTHONPATH=\"$dir/../lib:$dir/../python:$PYTHONPATH\" \\
exec \"${Python_EXECUTABLE}\" \"$dir/../examples/python/main.py\" \"$@\"
"
        FILE_PERMISSIONS
            OWNER_READ OWNER_WRITE OWNER_EXECUTE
            GROUP_READ GROUP_EXECUTE
            WORLD_READ WORLD_EXECUTE)
    install(
        PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/djv-python-install
        DESTINATION bin
        RENAME djv-python)
endif()

# Installed on every platform: the launcher runs it from here, and it is the
# worked example the documentation points at.
install(
    DIRECTORY ${PROJECT_SOURCE_DIR}/examples/python
    DESTINATION examples)
