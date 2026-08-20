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

# The commit and build date, refreshed on every build rather than only when
# the project is configured: a hash that is one commit stale is worse than
# none, since the point of it is telling two builds apart.
set(BUILD_INFO_H ${CMAKE_CURRENT_BINARY_DIR}/BuildInfo.h)
set(BUILD_INFO_ARGS
    -DSRC_DIR=${PROJECT_SOURCE_DIR}
    -DIN_FILE=${CMAKE_CURRENT_SOURCE_DIR}/BuildInfo.h.in
    -DOUT_FILE=${BUILD_INFO_H}
    -P ${PROJECT_SOURCE_DIR}/cmake/Modules/BuildInfo.cmake)
execute_process(COMMAND ${CMAKE_COMMAND} ${BUILD_INFO_ARGS})
add_custom_target(djvBuildInfo
    COMMAND ${CMAKE_COMMAND} ${BUILD_INFO_ARGS}
    BYPRODUCTS ${BUILD_INFO_H}
    COMMENT "Build information")
set_target_properties(djvBuildInfo PROPERTIES FOLDER lib)

add_library(djvModels ${HEADERS} ${HEADERS_PRIVATE} ${SOURCE})
add_dependencies(djvModels djvBuildInfo)

target_include_directories(djvModels
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/lib>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR})

# tlUI for the thumbnail system: a shared djvModels must name every
# library it uses directly, where the static build let the final link
# resolve it.
target_link_libraries(djvModels djvResource tlRender::tlUI tlRender::tlTimeline)

set_target_properties(djvModels PROPERTIES FOLDER lib)
set_target_properties(djvModels PROPERTIES PUBLIC_HEADER "${HEADERS}")

install(
    TARGETS djvModels
    EXPORT djvModelsTargets
    ARCHIVE DESTINATION lib COMPONENT dev
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
    PUBLIC_HEADER DESTINATION include/djv/Models COMPONENT dev)
install(
    EXPORT djvModelsTargets
    FILE djvModelsTargets.cmake
    DESTINATION "lib/cmake/djv"
    NAMESPACE djv::
    COMPONENT dev)
