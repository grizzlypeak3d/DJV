set(HEADERS
    AnnotationsModelTest.h
    AudioModelTest.h
    FilesModelTest.h
    ModelsTestUtil.h
    NotesModelTest.h
    PlaylistTest.h
    RecentFilesModelTest.h
    TimeUnitsModelTest.h
    ToolsModelTest.h
    ViewportModelTest.h)

set(SOURCE
    AnnotationsModelTest.cpp
    AudioModelTest.cpp
    FilesModelTest.cpp
    NotesModelTest.cpp
    PlaylistTest.cpp
    RecentFilesModelTest.cpp
    TimeUnitsModelTest.cpp
    ToolsModelTest.cpp
    ViewportModelTest.cpp)

# Static, for the reason in ftk's TestLib.
add_library(djvModelsTest STATIC ${SOURCE} ${HEADERS})
set_target_properties(djvModelsTest PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(djvModelsTest
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/lib>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/tests>
        $<INSTALL_INTERFACE:include>)

target_link_libraries(djvModelsTest djvModels ftkTestLib)

set_target_properties(djvModelsTest PROPERTIES FOLDER tests)
