set(HEADERS
    AudioModelTest.h
    FilesModelTest.h
    ModelsTestUtil.h
    RecentFilesModelTest.h
    TimeUnitsModelTest.h
    ToolsModelTest.h
    ViewportModelTest.h)

set(SOURCE
    AudioModelTest.cpp
    FilesModelTest.cpp
    RecentFilesModelTest.cpp
    TimeUnitsModelTest.cpp
    ToolsModelTest.cpp
    ViewportModelTest.cpp)

add_library(djvModelsTest ${SOURCE} ${HEADERS})

target_include_directories(djvModelsTest
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/lib>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/tests>
        $<INSTALL_INTERFACE:include>)

target_link_libraries(djvModelsTest djvModels ftkTestLib)

set_target_properties(djvModelsTest PROPERTIES FOLDER tests)
