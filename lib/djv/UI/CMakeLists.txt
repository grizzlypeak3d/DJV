set(HEADERS
    AboutDialog.h
    AudioPopup.h
    ColorWidgets.h
    FileThumbnail.h
    FrameRangePopup.h
    Init.h
    SeparateAudioDialog.h
    SettingsWidgets.h
    SetupDialog.h
    SpeedPopup.h
    StatusIndicator.h
    StatusIndicatorPopup.h
    SysInfoDialog.h
    ExportWidget.h
    ExportWidgets.h
    InfoWidget.h
    MagnifyWidget.h
    ViewWidgets.h
    Viewport.h)
set(HEADERS_PRIVATE)

set(SOURCE
    AboutDialog.cpp
    AudioPopup.cpp
    ColorWidgets.cpp
    FileThumbnail.cpp
    FrameRangePopup.cpp
    Init.cpp
    SeparateAudioDialog.cpp
    SettingsWidgets.cpp
    SetupDialog.cpp
    ShortcutsWidget.cpp
    SpeedPopup.cpp
    StatusIndicator.cpp
    StatusIndicatorPopup.cpp
    StyleWidget.cpp
    SysInfoDialog.cpp
    ExportWidget.cpp
    ExportWidgets.cpp
    InfoWidget.cpp
    MagnifyWidget.cpp
    ViewWidgets.cpp
    Viewport.cpp)

add_library(djvUI ${HEADERS} ${HEADERS_PRIVATE} ${SOURCE})
# Private when building, so this library's own API is dllexport here and
# dllimport to everything else. Public when static, which a consumer does
# have to know: the API then carries no declspec at all.
if(BUILD_SHARED_LIBS)
    target_compile_definitions(djvUI PRIVATE DJV_UI_EXPORTS)
else()
    target_compile_definitions(djvUI PUBLIC DJV_UI_STATIC)
endif()
target_link_libraries(djvUI djvModels djvResource tlRender::tlUI)
set_target_properties(djvUI PROPERTIES FOLDER lib)
set_target_properties(djvUI PROPERTIES PUBLIC_HEADER "${HEADERS}")

install(
    TARGETS djvUI
    EXPORT djvUITargets
    ARCHIVE DESTINATION lib COMPONENT dev
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
    PUBLIC_HEADER DESTINATION include/djv/UI COMPONENT dev)
install(
    EXPORT djvUITargets
    FILE djvUITargets.cmake
    DESTINATION "lib/cmake/djv"
    NAMESPACE djv::
    COMPONENT dev)
