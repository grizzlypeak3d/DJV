add_subdirectory(djv-test)

if(DJV_PYTHON)
    add_test(
        NAME djvPy-test
        COMMAND ${Python3_EXECUTABLE} -m unittest discover -v ${CMAKE_CURRENT_SOURCE_DIR})
    # The just-built modules rather than an external PYTHONPATH, so a plain
    # ctest run works anywhere; OTIO's Python package is installed by the
    # superbuild. The flag crosses to Python as an environment variable,
    # where the tests that need OpenGL read it and skip.
    set(PYTHON_TEST_ENV
        "PYTHONPATH=path_list_append:$<TARGET_FILE_DIR:ftkPy>"
        "PYTHONPATH=path_list_append:$<TARGET_FILE_DIR:tlRenderPy>"
        "PYTHONPATH=path_list_append:$<TARGET_FILE_DIR:djvPy>"
        "PYTHONPATH=path_list_append:${CMAKE_INSTALL_PREFIX}/python")
    if(DJV_TESTS_NO_GL)
        list(APPEND PYTHON_TEST_ENV "DJV_TESTS_NO_GL=set:1")
    endif()
    # A hang should fail rather than run until the job is killed, the same as
    # the C++ suite next door.
    set_tests_properties(djvPy-test PROPERTIES
        ENVIRONMENT_MODIFICATION "${PYTHON_TEST_ENV}"
        TIMEOUT 900)
endif()
