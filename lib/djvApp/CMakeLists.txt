set(HEADERS_ACTIONS
    Actions/AudioActions.h
    Actions/ColorActions.h
    Actions/CompareActions.h
    Actions/FileActions.h
    Actions/FrameActions.h
    Actions/HelpActions.h
    Actions/IActions.h
    Actions/PlaybackActions.h
    Actions/TimelineActions.h
    Actions/ToolsActions.h
    Actions/ViewActions.h
    Actions/WindowActions.h)
set(HEADERS_MENUS
    Menus/AudioMenu.h
    Menus/ColorMenu.h
    Menus/CompareMenu.h
    Menus/FileMenu.h
    Menus/FrameMenu.h
    Menus/HelpMenu.h
    Menus/PlaybackMenu.h
    Menus/TimelineMenu.h
    Menus/ToolsMenu.h
    Menus/ViewMenu.h
    Menus/WindowMenu.h)
set(HEADERS_MODELS
    Models/AudioModel.h
    Models/ColorModel.h
    Models/FilesModel.h
    Models/OCIOModel.h
    Models/RecentFilesModel.h
    Models/SettingsModel.h
    Models/TimeUnitsModel.h
    Models/ToolsModel.h
    Models/ViewportModel.h)
if(TLRENDER_BMD)
    list(APPEND HEADERS_MODELS Models/BMDDevicesModel.h)
endif()
set(HEADERS_TOOLS
    Tools/AudioTool.h
    Tools/ColorPickerTool.h
    Tools/ColorTool.h
    Tools/DevicesTool.h
    Tools/ExportTool.h
    Tools/FilesTool.h
    Tools/IToolWidget.h
    Tools/InfoTool.h
    Tools/MagnifyTool.h
    Tools/MessagesTool.h
    Tools/SettingsTool.h
    Tools/SysLogTool.h
    Tools/ToolsWidget.h
    Tools/ViewTool.h)
set(HEADERS_PRIVATE_TOOLS
    Tools/ColorToolPrivate.h
    Tools/FilesToolPrivate.h
    Tools/SettingsToolPrivate.h
    Tools/ViewToolPrivate.h)
set(HEADERS_WIDGETS
    Widgets/AboutDialog.h
    Widgets/AudioPopup.h
    Widgets/BottomToolBar.h
    Widgets/CompareToolBar.h
    Widgets/FileToolBar.h
    Widgets/SeparateAudioDialog.h
    Widgets/SetupDialog.h
    Widgets/ShuttleWidget.h
    Widgets/SpeedPopup.h
    Widgets/StatusBar.h
    Widgets/SysInfoDialog.h
    Widgets/TabBar.h
    Widgets/ToolsToolBar.h
    Widgets/ViewToolBar.h
    Widgets/Viewport.h
    Widgets/WindowToolBar.h)
set(HEADERS
    App.h
    MainWindow.h
    SecondaryWindow.h
    Shortcuts.h
    ${HEADERS_ACTIONS}
    ${HEADERS_MENUS}
    ${HEADERS_MODELS}
    ${HEADERS_TOOLS}
    ${HEADERS_WIDGETS})
set(HEADERS_PRIVATE
    ${HEADERS_PRIVATE_TOOLS}
    ${HEADERS_PRIVATE_WIDGETS})

set(SOURCE_ACTIONS
    Actions/AudioActions.cpp
    Actions/ColorActions.cpp
    Actions/CompareActions.cpp
    Actions/FileActions.cpp
    Actions/FrameActions.cpp
    Actions/HelpActions.cpp
    Actions/IActions.cpp
    Actions/PlaybackActions.cpp
    Actions/TimelineActions.cpp
    Actions/ToolsActions.cpp
    Actions/ViewActions.cpp
    Actions/WindowActions.cpp)
set(SOURCE_MENUS
    Menus/AudioMenu.cpp
    Menus/ColorMenu.cpp
    Menus/CompareMenu.cpp
    Menus/FileMenu.cpp
    Menus/FrameMenu.cpp
    Menus/HelpMenu.cpp
    Menus/PlaybackMenu.cpp
    Menus/TimelineMenu.cpp
    Menus/ToolsMenu.cpp
    Menus/ViewMenu.cpp
    Menus/WindowMenu.cpp)
set(SOURCE_MODELS
    Models/AudioModel.cpp
    Models/ColorModel.cpp
    Models/FilesModel.cpp
    Models/OCIOModel.cpp
    Models/RecentFilesModel.cpp
    Models/SettingsModel.cpp
    Models/TimeUnitsModel.cpp
    Models/ToolsModel.cpp
    Models/ViewportModel.cpp)
if(TLRENDER_BMD)
    list(APPEND SOURCE_MODELS Models/BMDDevicesModel.cpp)
endif()
set(SOURCE_TOOLS
    Tools/AudioTool.cpp
    Tools/ColorPickerTool.cpp
    Tools/ColorTool.cpp
    Tools/DevicesTool.cpp
    Tools/ExportTool.cpp
    Tools/FileButton.cpp
    Tools/FilesTool.cpp
    Tools/IToolWidget.cpp
    Tools/InfoTool.cpp
    Tools/MagnifyTool.cpp
    Tools/MessagesTool.cpp
    Tools/SettingsTool.cpp
    Tools/ShortcutsWidget.cpp
    Tools/StyleWidget.cpp
    Tools/SysLogTool.cpp
    Tools/ToolsWidget.cpp
    Tools/ViewTool.cpp)
set(SOURCE_WIDGETS
    Widgets/AboutDialog.cpp
    Widgets/AudioPopup.cpp
    Widgets/BottomToolBar.cpp
    Widgets/CompareToolBar.cpp
    Widgets/FileToolBar.cpp
    Widgets/SeparateAudioDialog.cpp
    Widgets/SeparateAudioWidget.cpp
    Widgets/SetupDialog.cpp
    Widgets/ShuttleWidget.cpp
    Widgets/SpeedPopup.cpp
    Widgets/StatusBar.cpp
    Widgets/SysInfoDialog.cpp
    Widgets/TabBar.cpp
    Widgets/ToolsToolBar.cpp
    Widgets/ViewToolBar.cpp
    Widgets/Viewport.cpp
    Widgets/WindowToolBar.cpp)
set(SOURCE
    App.cpp
    MainWindow.cpp
    SecondaryWindow.cpp
    Shortcuts.cpp
    ${SOURCE_ACTIONS}
    ${SOURCE_MENUS}
    ${SOURCE_MODELS}
    ${SOURCE_TOOLS}
    ${SOURCE_WIDGETS})

source_group("Actions Header Files" FILES ${HEADERS_ACTIONS})
source_group("Actions Source Files" FILES ${SOURCE_ACTIONS})
source_group("Menus Header Files" FILES ${HEADERS_MENUS})
source_group("Menus Source Files" FILES ${SOURCE_MENUS})
source_group("Models Header Files" FILES ${HEADERS_MODELS})
source_group("Models Source Files" FILES ${SOURCE_MODELS})
source_group("Tools Header Files" FILES ${HEADERS_TOOLS} ${HEADERS_PRIVATE_TOOLS})
source_group("Tools Source Files" FILES ${SOURCE_TOOLS})
source_group("Widgets Header Files" FILES ${HEADERS_WIDGETS} ${HEADERS_PRIVATE_WIDGETS})
source_group("Widgets Source Files" FILES ${SOURCE_WIDGETS})

add_library(djvApp ${HEADERS} ${HEADERS_PRIVATE} ${SOURCE})
target_link_libraries(djvApp djvResource tlRender::tlUI tlRender::tlDevice)
set_target_properties(djvApp PROPERTIES FOLDER lib)
set_target_properties(djvApp PROPERTIES PUBLIC_HEADER "${HEADERS}")

install(
    TARGETS djvApp
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
    PUBLIC_HEADER DESTINATION include/djvApp)
