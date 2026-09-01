set(HEADERS
    App.h
    AudioActions.h
    AudioMenu.h
    AudioTool.h
    BottomToolBar.h
    Benchmark.h
    Capture.h
    ColorActions.h
    ColorMenu.h
    ColorPickerTool.h
    ColorTool.h
    CompareActions.h
    CompareMenu.h
    CompareToolBar.h
    DiagTool.h
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
    RangeNameDialog.h
    ReviewActions.h
    ReviewMenu.h
    ReviewTool.h
    SaveReviewDialog.h
    SecondaryWindow.h
    SettingsTool.h
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
set(HEADERS_PRIVATE)

set(SOURCE
    App.cpp
    AudioActions.cpp
    AudioMenu.cpp
    AudioTool.cpp
    BottomToolBar.cpp
    Benchmark.cpp
    Capture.cpp
    ColorActions.cpp
    ColorMenu.cpp
    ColorPickerTool.cpp
    ColorTool.cpp
    CompareActions.cpp
    CompareMenu.cpp
    CompareToolBar.cpp
    DiagTool.cpp
    ExportTool.cpp
    FileActions.cpp
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
    RangeNameDialog.cpp
    ReviewActions.cpp
    ReviewMenu.cpp
    ReviewTool.cpp
    SaveReviewDialog.cpp
    SecondaryWindow.cpp
    SettingsTool.cpp
    StatusBar.cpp
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
    ViewTool.cpp)

add_library(djvApp ${HEADERS} ${HEADERS_PRIVATE} ${SOURCE})
# Private when building, so this library's own API is dllexport here and
# dllimport to everything else. Public when static, which a consumer does
# have to know: the API then carries no declspec at all.
if(BUILD_SHARED_LIBS)
    target_compile_definitions(djvApp PRIVATE DJV_APP_EXPORTS)
else()
    target_compile_definitions(djvApp PUBLIC DJV_APP_STATIC)
endif()
target_link_libraries(djvApp djvUI)
set_target_properties(djvApp PROPERTIES FOLDER lib)
set_target_properties(djvApp PROPERTIES PUBLIC_HEADER "${HEADERS}")

install(
    TARGETS djvApp
    EXPORT djvAppTargets
    ARCHIVE DESTINATION lib COMPONENT dev
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
    PUBLIC_HEADER DESTINATION include/djv/App COMPONENT dev)
install(
    EXPORT djvAppTargets
    FILE djvAppTargets.cmake
    DESTINATION "lib/cmake/djv"
    NAMESPACE djv::
    COMPONENT dev)

