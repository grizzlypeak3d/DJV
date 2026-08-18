set(HEADERS
    AboutDialog.h
    AudioPopup.h
    ColorWidgets.h
    FileThumbnail.h
    FrameRangePopup.h
    SeparateAudioDialog.h
    SettingsWidgets.h
    SetupDialog.h
    SpeedPopup.h
    StatusIndicatorPopup.h
    SysInfoDialog.h
    ViewWidgets.h)
set(HEADERS_PRIVATE)

set(SOURCE
    AboutDialog.cpp
    AudioPopup.cpp
    ColorWidgets.cpp
    FileThumbnail.cpp
    FrameRangePopup.cpp
    SeparateAudioDialog.cpp
    SettingsWidgets.cpp
    SetupDialog.cpp
    ShortcutsWidget.cpp
    SpeedPopup.cpp
    StatusIndicatorPopup.cpp
    StyleWidget.cpp
    SysInfoDialog.cpp
    ViewWidgets.cpp)

add_library(djvUI ${HEADERS} ${HEADERS_PRIVATE} ${SOURCE})
target_link_libraries(djvUI djvModels tlRender::tlUI)
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
