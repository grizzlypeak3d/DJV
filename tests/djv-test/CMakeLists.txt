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
