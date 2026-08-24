pybind11_add_module(djvPy Bindings.cpp)

target_include_directories(djvPy PRIVATE ${PROJECT_SOURCE_DIR}/lib)

target_link_libraries(djvPy PRIVATE djvModelsPy djvUIPy)

target_compile_options(djvPy PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/bigobj>)

set_target_properties(djvPy PROPERTIES FOLDER lib)

# The rpath convention of the executables names the interpreter's
# directory here, since the "executable" loading a Python module is
# Python itself: the module finds the libraries beside it instead.
if(APPLE)
    set_target_properties(djvPy PROPERTIES INSTALL_RPATH "@loader_path")
elseif(UNIX)
    set_target_properties(djvPy PROPERTIES INSTALL_RPATH "$ORIGIN")
endif()


install(TARGETS djvPy
    ARCHIVE DESTINATION lib COMPONENT dev
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin)

if(NOT WIN32)
    # A launcher that knows where the modules and the interpreter are, so
    # running the Python application is one command rather than an
    # environment to remember. The build tree names the module directories
    # directly; the installed copy finds everything beside itself.
    file(GENERATE
        OUTPUT ${CMAKE_BINARY_DIR}/bin/djv-python
        CONTENT "#!/bin/sh
PYTHONPATH=\"$<TARGET_FILE_DIR:ftkPy>:$<TARGET_FILE_DIR:tlRenderPy>:$<TARGET_FILE_DIR:djvPy>:${CMAKE_INSTALL_PREFIX}/python:$PYTHONPATH\" \\
exec \"${Python3_EXECUTABLE}\" \"${PROJECT_SOURCE_DIR}/examples/python/djv.py\" \"$@\"
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
exec \"${Python3_EXECUTABLE}\" \"$dir/../examples/python/djv.py\" \"$@\"
"
        FILE_PERMISSIONS
            OWNER_READ OWNER_WRITE OWNER_EXECUTE
            GROUP_READ GROUP_EXECUTE
            WORLD_READ WORLD_EXECUTE)
    install(
        PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/djv-python-install
        DESTINATION bin
        RENAME djv-python)
    install(
        DIRECTORY ${PROJECT_SOURCE_DIR}/examples/python
        DESTINATION examples)
endif()
