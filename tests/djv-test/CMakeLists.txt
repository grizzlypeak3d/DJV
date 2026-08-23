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

# OpenTimelineIO's libraries are not in bin. Building its Python package puts
# them beside the module instead, in python/opentimelineio, which is what the
# package needs and is nowhere the loader looks. This links opentimelineio, so
# without the directory on PATH the program does not start, and on Windows a
# missing library is a message box rather than an error -- on a continuous
# integration runner that is a test that never finishes.
#
# On PATH rather than copied into bin: two images of opentimelineio in one
# process do not share their type information, which is the thing shared
# libraries were turned on to avoid.
set_tests_properties(djv-test PROPERTIES ENVIRONMENT_MODIFICATION
    "PATH=path_list_prepend:${CMAKE_INSTALL_PREFIX}/python/opentimelineio")
