add_executable(djv-test main.cpp)

target_include_directories(djv-test
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/lib>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/tests>
        $<INSTALL_INTERFACE:include>)

target_link_libraries(djv-test
    djvModelsTest)

set_target_properties(djv-test PROPERTIES FOLDER tests)

add_test(djv-test ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/djv-test${CMAKE_EXECUTABLE_SUFFIX})
