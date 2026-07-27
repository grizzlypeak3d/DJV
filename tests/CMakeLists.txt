add_subdirectory(djv-test)

add_executable(djvPlaylistModelTest PlaylistModelTest.cpp)
target_link_libraries(
    djvPlaylistModelTest
    djvModels
    tlRender::tlUI
    ftk::ftkUI)
target_compile_definitions(
    djvPlaylistModelTest
    PRIVATE DJV_TEST_SAMPLE_DATA="${CMAKE_SOURCE_DIR}/etc/SampleData")
set_target_properties(djvPlaylistModelTest PROPERTIES FOLDER tests)

add_test(
    NAME djvPlaylistModelTest
    COMMAND $<TARGET_FILE:djvPlaylistModelTest>)
