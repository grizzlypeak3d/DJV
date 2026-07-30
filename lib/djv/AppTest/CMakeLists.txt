add_executable(
    djvWindowsOpenBrokerTest
    WindowsOpenBrokerTest.cpp
    ../App/WindowsOpenBroker.cpp)
target_compile_definitions(
    djvWindowsOpenBrokerTest
    PRIVATE
        DJV_WINDOWS_OPEN_BROKER_TEST_MAIN
        DJV_WINDOWS_OPEN_BROKER_TESTING)
target_include_directories(
    djvWindowsOpenBrokerTest
    PRIVATE ${PROJECT_SOURCE_DIR}/lib)
set_target_properties(djvWindowsOpenBrokerTest PROPERTIES FOLDER tests)
add_test(NAME djv-windows-open-broker COMMAND djvWindowsOpenBrokerTest)
set_tests_properties(
    djv-windows-open-broker
    PROPERTIES LABELS "unit;app;windows;security" TIMEOUT 90)
