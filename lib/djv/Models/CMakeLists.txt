set(HEADERS
    AudioModel.h
    ColorModel.h
    FilesModel.h
    OCIOModel.h
    RecentFilesModel.h
    SettingsModel.h
    TimeUnitsModel.h
    ToolsModel.h
    ViewportModel.h)
if(TLRENDER_BMD)
    list(APPEND HEADERS BMDDevicesModel.h)
endif()
set(HEADERS_PRIVATE)

set(SOURCE
    AudioModel.cpp
    ColorModel.cpp
    FilesModel.cpp
    OCIOModel.cpp
    RecentFilesModel.cpp
    SettingsModel.cpp
    TimeUnitsModel.cpp
    ToolsModel.cpp
    ViewportModel.cpp)
if(TLRENDER_BMD)
    list(APPEND SOURCE BMDDevicesModel.cpp)
endif()

add_library(djvModels ${HEADERS} ${HEADERS_PRIVATE} ${SOURCE})
target_link_libraries(djvModels djvCore tlRender::tlTimeline tlRender::tlDevice)
set_target_properties(djvModels PROPERTIES FOLDER lib)
set_target_properties(djvModels PROPERTIES PUBLIC_HEADER "${HEADERS}")

install(
    TARGETS djvModels
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
    PUBLIC_HEADER DESTINATION include/djvModels)
