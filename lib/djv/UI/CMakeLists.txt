set(HEADERS
    AboutDialog.h
    AudioPopup.h
    SeparateAudioDialog.h
    SettingsWidgets.h
    SetupDialog.h
    SpeedPopup.h
    SysInfoDialog.h)
set(HEADERS_PRIVATE)

set(SOURCE
    AboutDialog.cpp
    AudioPopup.cpp
    SeparateAudioDialog.cpp
    SeparateAudioWidget.cpp
    SettingsWidgets.cpp
    SetupDialog.cpp
    SpeedPopup.cpp
    SysInfoDialog.cpp)

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
