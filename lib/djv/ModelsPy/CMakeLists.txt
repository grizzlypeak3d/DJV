set(HEADERS
    Bindings.h)
set(HEADERS_PRIVATE)
set(SOURCE
    AppInfoModel.cpp
    AudioModel.cpp
    Bindings.cpp
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

add_library(djvModelsPy ${HEADERS} ${HEADERS_PRIVATE} ${SOURCE})

target_include_directories(djvModelsPy
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/lib>
        $<INSTALL_INTERFACE:include>)

# ftkUI directly: the bindings name ftk::Settings and ftk::RecentFilesModel,
# which djvModels does not carry in its link interface.
target_link_libraries(djvModelsPy djvModels ftkUI Python3::Python)

# The settings bindings alone go past what a COFF object can hold, which is
# what pybind11 does to a translation unit that names a lot of types:
#
#     error C1128: number of sections exceeded object file format limit
target_compile_options(djvModelsPy PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/bigobj>)

set_target_properties(djvModelsPy PROPERTIES FOLDER lib)
set_target_properties(djvModelsPy PROPERTIES PUBLIC_HEADER "${HEADERS}")

install(TARGETS djvModelsPy
    ARCHIVE DESTINATION lib COMPONENT dev
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
    PUBLIC_HEADER DESTINATION include/djv/ModelsPy COMPONENT dev)
