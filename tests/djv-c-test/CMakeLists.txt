# The C interface to the models, from C. The settings file is under the
# build directory, removed before each run by the fixture, so the models
# start from their defaults and the test reads back only what it wrote.
add_executable(djv-c-test djv-c-test.c)
target_link_libraries(djv-c-test djvC)
target_compile_definitions(djv-c-test PRIVATE
    DJV_SAMPLE_DATA="${PROJECT_SOURCE_DIR}/etc/SampleData"
    DJV_C_TEST_SETTINGS="${CMAKE_CURRENT_BINARY_DIR}/djv-c-test-settings.json"
    DJV_C_TEST_DIR="${CMAKE_CURRENT_BINARY_DIR}")
set_target_properties(djv-c-test PROPERTIES
    FOLDER tests
    C_STANDARD 11
    C_STANDARD_REQUIRED ON)

add_test(
    NAME djv-c-test-settings
    COMMAND ${CMAKE_COMMAND} -E rm -f
        ${CMAKE_CURRENT_BINARY_DIR}/djv-c-test-settings.json
        ${CMAKE_CURRENT_BINARY_DIR}/review.djvr
        ${CMAKE_CURRENT_BINARY_DIR}/empty.djvr
        ${CMAKE_CURRENT_BINARY_DIR}/bad.djvr)
set_tests_properties(djv-c-test-settings PROPERTIES FIXTURES_SETUP djv-c-test-clean)
add_test(NAME djv-c-test COMMAND djv-c-test)
# No sound card on a runner: SDL's dummy audio driver. OpenTimelineIO's
# libraries are on PATH for the reason in djv-test.
set_tests_properties(djv-c-test PROPERTIES
    FIXTURES_REQUIRED djv-c-test-clean
    ENVIRONMENT "SDL_AUDIO_DRIVER=dummy"
    ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:${CMAKE_INSTALL_PREFIX}/python/opentimelineio")
