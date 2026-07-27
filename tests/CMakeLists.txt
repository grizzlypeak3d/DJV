add_subdirectory(djv-test)

add_executable(
    djvTimelineLoadServiceTest
    TimelineLoadServiceTest.cpp)
target_compile_definitions(
    djvTimelineLoadServiceTest
    PRIVATE FTK_ASSERT)
target_include_directories(
    djvTimelineLoadServiceTest
    PRIVATE ${PROJECT_SOURCE_DIR}/lib)
target_link_libraries(djvTimelineLoadServiceTest djvApp)
set_target_properties(djvTimelineLoadServiceTest PROPERTIES FOLDER tests)
add_test(
    NAME djv-timeline-load-service
    COMMAND djvTimelineLoadServiceTest)
set_tests_properties(
    djv-timeline-load-service
    PROPERTIES LABELS "unit;app;concurrency;cancellation" TIMEOUT 30)
