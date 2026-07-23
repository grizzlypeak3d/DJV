set(HEADERS
    AppInfoModel.h
    AudioModel.h
    ColorModel.h
    CommandsModel.h
    FilesModel.h
    OCIOModel.h
    RecentFilesModel.h
    SettingsModel.h
    Shortcuts.h
    TimeUnitsModel.h
    ToolsModel.h
    Version.h
    ViewportModel.h)
set(HEADERS_PRIVATE)

set(SOURCE
    AppInfoModel.cpp
    AudioModel.cpp
    ColorModel.cpp
    CommandsModel.cpp
    FilesModel.cpp
    OCIOModel.cpp
    RecentFilesModel.cpp
    SettingsModel.cpp
    Shortcuts.cpp
    TimeUnitsModel.cpp
    ToolsModel.cpp
    ViewportModel.cpp)

add_library(djvModels ${HEADERS} ${HEADERS_PRIVATE} ${SOURCE})

target_include_directories(djvModels
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/lib>
        $<INSTALL_INTERFACE:include>)

target_link_libraries(djvModels djvResource tlRender::tlTimeline)

set_target_properties(djvModels PROPERTIES FOLDER lib)
set_target_properties(djvModels PROPERTIES PUBLIC_HEADER "${HEADERS}")

install(
    TARGETS djvModels
    EXPORT djvModelsTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
    PUBLIC_HEADER DESTINATION include/djv/Models)
install(
    EXPORT djvModelsTargets
    FILE djvModelsTargets.cmake
    DESTINATION "lib/cmake/djv"
    NAMESPACE djv::)
