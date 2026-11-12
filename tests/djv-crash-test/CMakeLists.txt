# The crash log: a program that ends each way, and the line it leaves in
# the log (check.cmake).
add_executable(djv-crash-test djv-crash-test.cpp)
target_link_libraries(djv-crash-test djvApp)
set_target_properties(djv-crash-test PROPERTIES FOLDER tests)

foreach(how clean error crash terminate)
    add_test(
        NAME djv-crash-test-${how}
        COMMAND ${CMAKE_COMMAND}
            -DPROGRAM=$<TARGET_FILE:djv-crash-test>
            -DHOW=${how}
            -DLOG=${CMAKE_CURRENT_BINARY_DIR}/djv-crash-test-${how}.log
            -P ${CMAKE_CURRENT_SOURCE_DIR}/check.cmake)
    # OpenTimelineIO's libraries are on PATH for the reason in djv-test.
    set_tests_properties(djv-crash-test-${how} PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:${CMAKE_INSTALL_PREFIX}/python/opentimelineio")
endforeach()
