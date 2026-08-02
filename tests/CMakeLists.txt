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

add_executable(
    djvFileFilterTest
    ../lib/djv/ModelsTest/FileFilterTest.cpp
    ../lib/djv/Models/FileFilter.cpp
    ../lib/djv/Models/FolderScanner.cpp)
target_compile_definitions(
    djvFileFilterTest
    PRIVATE DJV_FILE_FILTER_TEST_STANDALONE FTK_ASSERT)
target_include_directories(
    djvFileFilterTest
    PRIVATE ${PROJECT_SOURCE_DIR}/lib)
target_link_libraries(djvFileFilterTest ftkCore)
set_target_properties(djvFileFilterTest PROPERTIES FOLDER tests)
add_test(NAME djv-file-filter COMMAND djvFileFilterTest)
set_tests_properties(
    djv-file-filter
    PROPERTIES LABELS "unit;models;filesystem" TIMEOUT 30)

add_executable(
    djvFolderScanServiceTest
    ../lib/djv/AppTest/FolderScanServiceTest.cpp
    ../lib/djv/App/FolderScanService.cpp
    ../lib/djv/Models/FileFilter.cpp
    ../lib/djv/Models/FolderScanner.cpp)
target_compile_definitions(
    djvFolderScanServiceTest
    PRIVATE DJV_FOLDER_SCAN_SERVICE_TEST_MAIN FTK_ASSERT)
target_include_directories(
    djvFolderScanServiceTest
    PRIVATE ${PROJECT_SOURCE_DIR}/lib)
target_link_libraries(djvFolderScanServiceTest ftkCore)
set_target_properties(djvFolderScanServiceTest PROPERTIES FOLDER tests)
add_test(NAME djv-folder-scan-service COMMAND djvFolderScanServiceTest)
set_tests_properties(
    djv-folder-scan-service
    PROPERTIES LABELS "unit;app;filesystem;concurrency" TIMEOUT 180)
