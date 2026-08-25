set(HEADERS
    Bindings.h)
set(HEADERS_PRIVATE)
set(SOURCE
    AudioPopup.cpp
    Bindings.cpp
    ColorWidgets.cpp
    ExportWidget.cpp
    FileThumbnail.cpp
    FrameRangePopup.cpp
    InfoWidget.cpp
    MagnifyWidget.cpp
    SeparateAudioDialog.cpp
    SettingsWidgets.cpp
    SpeedPopup.cpp
    StatusIndicator.cpp
    SysInfoDialog.cpp
    ViewWidgets.cpp
    Viewport.cpp)

# Static. The bindings are linked by djvPy and nothing else, so there is no
# API here for anything to import -- the module absorbs them.
add_library(djvUIPy STATIC ${HEADERS} ${HEADERS_PRIVATE} ${SOURCE})
set_target_properties(djvUIPy PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(djvUIPy
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/lib>
        $<INSTALL_INTERFACE:include>)

target_link_libraries(djvUIPy djvUI Python3::Python)

target_compile_options(djvUIPy PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/bigobj>)

set_target_properties(djvUIPy PROPERTIES FOLDER lib)
set_target_properties(djvUIPy PROPERTIES PUBLIC_HEADER "${HEADERS}")

install(TARGETS djvUIPy
    ARCHIVE DESTINATION lib COMPONENT dev
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
    PUBLIC_HEADER DESTINATION include/djv/UIPy COMPONENT dev)
