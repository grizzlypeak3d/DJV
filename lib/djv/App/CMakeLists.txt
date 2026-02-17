set(HEADERS
    App.h
    AudioActions.h
    AudioMenu.h
    AudioTool.h
    BottomToolBar.h
    ColorActions.h
    ColorMenu.h
    ColorPickerTool.h
    ColorTool.h
    CompareActions.h
    CompareMenu.h
    CompareToolBar.h
    DiagTool.h
    DevicesTool.h
    ExportTool.h
    FileActions.h
    FileMenu.h
    FileToolBar.h
    FilesTool.h
    FrameActions.h
    FrameMenu.h
    HelpActions.h
    HelpMenu.h
    IActions.h
    IToolWidget.h
    InfoTool.h
    MagnifyTool.h
    MainWindow.h
    MessagesTool.h
    PlaybackActions.h
    PlaybackMenu.h
    SecondaryWindow.h
    SettingsTool.h
    SetupDialog.h
    StatusBar.h
    SysLogTool.h
    TabBar.h
    TimelineActions.h
    TimelineMenu.h
    ToolsActions.h
    ToolsMenu.h
    ToolsToolBar.h
    ToolsWidget.h
    ViewActions.h
    ViewMenu.h
    ViewToolBar.h
    WindowActions.h
    WindowMenu.h
    WindowToolBar.h
    ViewTool.h)
set(HEADERS_PRIVATE
    ColorToolPrivate.h
    FilesToolPrivate.h
    SettingsToolPrivate.h
    ViewToolPrivate.h
    Viewport.h)

set(SOURCE
    App.cpp
    AudioActions.cpp
    AudioMenu.cpp
    AudioTool.cpp
    BottomToolBar.cpp
    ColorActions.cpp
    ColorMenu.cpp
    ColorPickerTool.cpp
    ColorTool.cpp
    CompareActions.cpp
    CompareMenu.cpp
    CompareToolBar.cpp
    DevicesTool.cpp
    DiagTool.cpp
    ExportTool.cpp
    FileActions.cpp
    FileButton.cpp
    FileMenu.cpp
    FileToolBar.cpp
    FilesTool.cpp
    FrameActions.cpp
    FrameMenu.cpp
    HelpActions.cpp
    HelpMenu.cpp
    IActions.cpp
    IToolWidget.cpp
    InfoTool.cpp
    MainWindow.cpp
    MagnifyTool.cpp
    MessagesTool.cpp
    PlaybackActions.cpp
    PlaybackMenu.cpp
    SecondaryWindow.cpp
    SettingsTool.cpp
    SetupDialog.cpp
    ShortcutsWidget.cpp
    StatusBar.cpp
    StyleWidget.cpp
    SysLogTool.cpp
    TabBar.cpp
    TimelineActions.cpp
    TimelineMenu.cpp
    ToolsActions.cpp
    ToolsMenu.cpp
    ToolsToolBar.cpp
    ToolsWidget.cpp
    ViewActions.cpp
    ViewMenu.cpp
    ViewToolBar.cpp
    WindowActions.cpp
    WindowMenu.cpp
    WindowToolBar.cpp
    ViewTool.cpp
    Viewport.cpp)

add_library(djvApp ${HEADERS} ${HEADERS_PRIVATE} ${SOURCE})
target_link_libraries(djvApp djvUI)
set_target_properties(djvApp PROPERTIES FOLDER lib)
set_target_properties(djvApp PROPERTIES PUBLIC_HEADER "${HEADERS}")

install(
    TARGETS djvApp
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
    PUBLIC_HEADER DESTINATION include/djvApp)
