add_executable(djv-test djv-test.h djv-test.cpp)

target_include_directories(djv-test
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/lib>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/tests>
        $<INSTALL_INTERFACE:include>)

target_link_libraries(djv-test
    djvModelsTest
    tlUI)

set_target_properties(djv-test PROPERTIES FOLDER tests)

add_test(
    NAME djv-test
    COMMAND djv-test -log)

# A test that hangs should fail, not run until the job is killed: this one
# waits on futures, and a reader that never answers waits forever. The whole
# suite is a couple of minutes, so this only fires on a hang.
set_tests_properties(djv-test PROPERTIES TIMEOUT 900)
