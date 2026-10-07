add_subdirectory(djv-test)

# Scripted sessions: each shot in capture/capture.json is a test that drives
# the application through the capture tool -- clicks, drags, keys, typed
# text -- and must end with its expectations met and nothing marked as an
# error in the log (see ftk::Capture and etc/Screenshots/notes.txt). The
# window is drawn offscreen, so no display is needed beyond what OpenGL
# needs; the Linux jobs run under xvfb. The screenshots land in the build
# tree, for looking at a failure.
if(NOT DJV_TESTS_NO_GL)
    set(DJV_CAPTURE_MANIFEST ${CMAKE_CURRENT_SOURCE_DIR}/capture/capture.json)
    set(DJV_CAPTURE_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/capture)
    file(MAKE_DIRECTORY ${DJV_CAPTURE_OUTPUT})
    file(READ ${DJV_CAPTURE_MANIFEST} DJV_CAPTURE_JSON)
    string(JSON DJV_CAPTURE_COUNT LENGTH ${DJV_CAPTURE_JSON} shots)
    math(EXPR DJV_CAPTURE_LAST "${DJV_CAPTURE_COUNT} - 1")
    foreach(i RANGE ${DJV_CAPTURE_LAST})
        string(JSON shot GET ${DJV_CAPTURE_JSON} shots ${i} id)
        add_test(
            NAME djv-capture-${shot}
            COMMAND djv
                -resetSettings
                -settingsFile ${DJV_CAPTURE_OUTPUT}/settings.json
                -captureManifest ${DJV_CAPTURE_MANIFEST}
                -captureShot ${shot}
                -captureOutput ${DJV_CAPTURE_OUTPUT}
                -log
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
        # The PATH entry: see djv-test/CMakeLists.txt. The audio driver: a
        # runner has no sound card, and a player that cannot open the audio
        # device logs an error, which a strict shot counts as a failure.
        # The shots do not play audio, so SDL's dummy driver serves them.
        # The video driver: the Linux jobs run under xvfb, where only X11
        # can work, and SDL probing Wayland first on a runner with no
        # session (no XDG_RUNTIME_DIR) crashed before the first log line.
        set(DJV_CAPTURE_ENV
            "PATH=path_list_prepend:${CMAKE_INSTALL_PREFIX}/python/opentimelineio"
            "SDL_AUDIODRIVER=set:dummy")
        if(UNIX AND NOT APPLE)
            list(APPEND DJV_CAPTURE_ENV "SDL_VIDEODRIVER=set:x11")
        endif()
        set_tests_properties(djv-capture-${shot} PROPERTIES
            ENVIRONMENT_MODIFICATION "${DJV_CAPTURE_ENV}")
    endforeach()
    # A shot added to the manifest is a test added, so configure again.
    set_property(DIRECTORY APPEND PROPERTY
        CMAKE_CONFIGURE_DEPENDS ${DJV_CAPTURE_MANIFEST})
endif()

if(DJV_PYTHON)
    # Through the runner rather than "-m unittest" directly: on Windows the
    # bindings' dependencies live in bin, and since Python 3.8 only
    # os.add_dll_directory() can point an extension module at them. PATH,
    # which the continuous integration jobs set, has no effect.
    # OpenTimelineIO's libraries are not in bin: its Python package keeps them
    # beside the module, which is what the package needs and where nothing
    # else looks. The bindings link opentimelineio directly, so the directory
    # has to be named here too or the import fails on Windows with
    #
    #     ImportError: DLL load failed while importing _tlrender:
    #     The specified module could not be found.
    add_test(
        NAME djv-python-test
        COMMAND ${Python_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/run-python-tests.py
            ${CMAKE_INSTALL_PREFIX}/bin
            ${CMAKE_INSTALL_PREFIX}/python/opentimelineio)
    # The just-built modules rather than an external PYTHONPATH, so a plain
    # ctest run works anywhere; OTIO's Python package is installed by the
    # superbuild. The flag crosses to Python as an environment variable,
    # where the tests that need OpenGL read it and skip.
    set(PYTHON_TEST_ENV
        "PYTHONPATH=path_list_append:$<TARGET_FILE_DIR:_feather_tk>/.."
        "PYTHONPATH=path_list_append:$<TARGET_FILE_DIR:_tlrender>/.."
        "PYTHONPATH=path_list_append:$<TARGET_FILE_DIR:_djv>/.."
        "PYTHONPATH=path_list_append:${CMAKE_INSTALL_PREFIX}/python")
    if(DJV_TESTS_NO_GL)
        list(APPEND PYTHON_TEST_ENV "DJV_TESTS_NO_GL=set:1")
    endif()
    set_tests_properties(djv-python-test PROPERTIES
        ENVIRONMENT_MODIFICATION "${PYTHON_TEST_ENV}")
endif()
