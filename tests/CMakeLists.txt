add_subdirectory(djv-test)

add_executable(djvRelativeViewTest RelativeViewTest.cpp)
target_link_libraries(djvRelativeViewTest djvApp)
set_target_properties(djvRelativeViewTest PROPERTIES FOLDER tests)
add_test(
    NAME djvRelativeViewTest
    COMMAND $<TARGET_FILE:djvRelativeViewTest>)
