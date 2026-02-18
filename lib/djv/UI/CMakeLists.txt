set(HEADERS
    AboutDialog.h
    AudioPopup.h
    ColorWidgets.h
    FileButton.h
    SeparateAudioDialog.h
    SettingsWidgets.h
    SetupDialog.h
    SpeedPopup.h
    SysInfoDialog.h
    ViewWidgets.h)
set(HEADERS_PRIVATE)

set(SOURCE
    AboutDialog.cpp
    AudioPopup.cpp
    ColorWidgets.cpp
    FileButton.cpp
    SeparateAudioDialog.cpp
    SettingsWidgets.cpp
    SetupDialog.cpp
    ShortcutsWidget.cpp
    SpeedPopup.cpp
    StyleWidget.cpp
    SysInfoDialog.cpp
    ViewWidgets.cpp)

add_library(djvUI ${HEADERS} ${HEADERS_PRIVATE} ${SOURCE})
target_link_libraries(djvUI djvModels tlRender::tlUI)
set_target_properties(djvUI PROPERTIES FOLDER lib)
set_target_properties(djvUI PROPERTIES PUBLIC_HEADER "${HEADERS}")

install(
    TARGETS djvUI
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
    PUBLIC_HEADER DESTINATION include/djvUI)
