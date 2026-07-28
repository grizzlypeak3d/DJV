add_subdirectory(djv-test)

add_executable(
    djv-playback-ui-state-test
    PlaybackUIStateTest.cpp
    ../lib/djv/App/PlaybackUIState.cpp)
target_include_directories(
    djv-playback-ui-state-test
    PRIVATE
        ../lib)
set_target_properties(
    djv-playback-ui-state-test
    PROPERTIES
        FOLDER tests)
add_test(
    NAME djv-playback-ui-state
    COMMAND djv-playback-ui-state-test)
