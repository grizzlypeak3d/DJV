// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/MainWindow.h>

#include <djv/App/ExportTool.h>

#include <djv/App/App.h>
#include <djv/Models/FilesModel.h>
#include <djv/App/AudioActions.h>
#include <djv/App/AudioMenu.h>
#include <djv/App/BottomToolBar.h>
#include <djv/App/ColorActions.h>
#include <djv/App/ColorMenu.h>
#include <djv/App/CompareActions.h>
#include <djv/App/CompareMenu.h>
#include <djv/App/CompareToolBar.h>
#include <djv/App/FileActions.h>
#include <djv/App/FileMenu.h>
#include <djv/App/FileToolBar.h>
#include <djv/App/FrameActions.h>
#include <djv/App/FrameMenu.h>
#include <djv/App/HelpActions.h>
#include <djv/App/HelpMenu.h>
#include <djv/App/PlaybackActions.h>
#include <djv/App/ReviewActions.h>
#include <djv/App/ReviewMenu.h>
#include <djv/App/ReviewTool.h>
#include <djv/App/PlaybackMenu.h>
#include <djv/App/StatusBar.h>

#include <ftk/Core/String.h>
#include <ftk/UI/Tooltip.h>

#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif // __APPLE__
#include <djv/App/TabBar.h>
#include <djv/App/TimelineActions.h>
#include <djv/App/TimelineMenu.h>
#include <djv/App/ToolsActions.h>
#include <djv/App/ToolsMenu.h>
#include <djv/App/ToolsToolBar.h>
#include <djv/App/ToolsWidget.h>
#include <djv/App/ViewActions.h>
#include <djv/App/ViewMenu.h>
#include <djv/App/ViewToolBar.h>
#include <djv/UI/TimelinePreview.h>
#include <djv/UI/TourOverlay.h>
#include <djv/UI/Viewport.h>
#include <djv/App/WindowActions.h>
#include <djv/App/WindowMenu.h>
#include <djv/App/WindowToolBar.h>
#include <djv/UI/AboutDialog.h>
#include <djv/UI/Init.h>
#include <djv/UI/SysInfoDialog.h>
#include <djv/Models/AppInfoModel.h>
#include <djv/Models/ColorModel.h>
#include <djv/Models/TimeUnitsModel.h>
#include <djv/Models/ToolsModel.h>
#include <djv/Models/ViewportModel.h>

#include <tlRender/UI/TimelineWidget.h>
#include <tlRender/GL/Render.h>

#include <ftk/UI/ButtonGroup.h>
#include <ftk/UI/Divider.h>
#include <ftk/UI/IconSystem.h>
#include <ftk/UI/Label.h>
#include <ftk/UI/Menu.h>
#include <ftk/UI/MenuBar.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/ScreenshotTag.h>
#include <ftk/UI/Splitter.h>
#include <ftk/UI/ToolButton.h>
#include <ftk/Core/Format.h>
#include <ftk/Core/Timer.h>

namespace djv
{
    namespace app
    {
        namespace
        {
            // A context menu of window chrome visibility toggles. The
            // actions are the Window menu's own, so the check marks stay in
            // sync and the toggles go through the same commands; the Window
            // menu remains the complete inventory, and these are a second
            // door to part of it.
            std::function<std::shared_ptr<ftk::Menu>(void)> chromeMenuCallback(
                const std::shared_ptr<ftk::Context>& context,
                const std::shared_ptr<WindowActions>& windowActions,
                const std::vector<std::string>& names)
            {
                std::weak_ptr<ftk::Context> contextWeak(context);
                std::weak_ptr<WindowActions> windowActionsWeak(windowActions);
                return
                    [contextWeak, windowActionsWeak, names]() ->
                    std::shared_ptr<ftk::Menu>
                    {
                        auto context = contextWeak.lock();
                        auto windowActions = windowActionsWeak.lock();
                        if (!context || !windowActions)
                            return nullptr;
                        auto out = ftk::Menu::create(context);
                        auto actions = windowActions->getActions();
                        for (const auto& name : names)
                        {
                            out->addAction(actions[name]);
                        }
                        return out;
                    };
            }
        }

        struct MainWindow::Private
        {
            std::weak_ptr<App> app;
            std::shared_ptr<models::SettingsModel> settingsModel;
            std::shared_ptr<ftk::Observable<bool> > presentMode;
            bool presentHint = false;
            std::shared_ptr<ftk::Timer> presentHintTimer;
            bool shown = false;

            std::shared_ptr<ui::Viewport> viewport;
            std::shared_ptr<tl::ui::TimelineWidget> timelineWidget;
            std::shared_ptr<FileActions> fileActions;
            std::shared_ptr<ReviewActions> reviewActions;
            std::shared_ptr<CompareActions> compareActions;
            std::shared_ptr<PlaybackActions> playbackActions;
            std::shared_ptr<FrameActions> frameActions;
            std::shared_ptr<TimelineActions> timelineActions;
            std::shared_ptr<AudioActions> audioActions;
            std::shared_ptr<ViewActions> viewActions;
            std::shared_ptr<WindowActions> windowActions;
            std::shared_ptr<ColorActions> colorActions;
            std::shared_ptr<ToolsActions> toolsActions;
            std::shared_ptr<HelpActions> helpActions;
            std::shared_ptr<FileMenu> fileMenu;
            std::shared_ptr<ReviewMenu> reviewMenu;
            std::shared_ptr<CompareMenu> compareMenu;
            std::shared_ptr<PlaybackMenu> playbackMenu;
            std::shared_ptr<FrameMenu> frameMenu;
            std::shared_ptr<TimelineMenu> timelineMenu;
            std::shared_ptr<AudioMenu> audioMenu;
            std::shared_ptr<ViewMenu> viewMenu;
            std::shared_ptr<WindowMenu> windowMenu;
            std::shared_ptr<ColorMenu> colorMenu;
            std::shared_ptr<ToolsMenu> toolsMenu;
            std::shared_ptr<HelpMenu> helpMenu;
            std::shared_ptr<ftk::MenuBar> menuBar;
            std::shared_ptr<ftk::Divider> menuBarDivider;
            std::shared_ptr<FileToolBar> fileToolBar;
            std::shared_ptr<CompareToolBar> compareToolBar;
            std::shared_ptr<ViewToolBar> viewToolBar;
            std::shared_ptr<WindowToolBar> windowToolBar;
            std::shared_ptr<ToolsToolBar> toolsToolBar;
            std::shared_ptr<TabBar> tabBar;
            std::shared_ptr<BottomToolBar> bottomToolBar;
            std::shared_ptr<StatusBar> statusBar;
            std::shared_ptr<ToolsWidget> toolsWidget;
            std::shared_ptr<ui::TourOverlay> tourOverlay;
            std::shared_ptr<ui::AboutDialog> aboutDialog;
            std::shared_ptr<ui::SysInfoDialog> sysInfoDialog;
            std::shared_ptr<ftk::HorizontalLayout> toolBarLayout;
            std::shared_ptr<ftk::Splitter> splitter;
            std::shared_ptr<ftk::Splitter> splitter2;
            std::shared_ptr<ftk::VerticalLayout> splitterLayout;
            std::shared_ptr<ftk::VerticalLayout> layout;

            std::shared_ptr<ftk::Observer<std::shared_ptr<tl::Player> > > playerObserver;
            std::shared_ptr<ftk::ListObserver<std::shared_ptr<models::FilesModelItem> > > filesObserver;
            std::shared_ptr<ftk::Observer<tl::CompareOptions> > compareOptionsObserver;
            std::shared_ptr<ftk::Observer<tl::OCIOOptions> > ocioOptionsObserver;
            std::shared_ptr<ftk::Observer<tl::LUTOptions> > lutOptionsObserver;
            std::shared_ptr<ftk::Observer<ftk::ImageType> > colorBufferObserver;
            std::shared_ptr<ftk::ListObserver<std::string> > openToolsObserver;
            std::shared_ptr<ftk::Observer<std::pair<std::string, std::string> > > showSectionObserver;
            std::shared_ptr<ftk::Observer<models::MouseSettings> > mouseSettingsObserver;
            std::shared_ptr<ftk::Observer<models::TimelineSettings> > timelineSettingsObserver;
            std::shared_ptr<ftk::Observer<bool> > timelineFrameViewObserver;

            //! The player the timeline shows, and the preview of it that
            //! hovering the timeline opens.
            std::shared_ptr<tl::Player> player;
            bool timelinePreview = true;
            std::shared_ptr<ui::TimelinePreview> preview;
            //! The hover waiting out the tooltip delay before the preview
            //! opens; once open it follows the cursor without one.
            std::optional<OTIO_NS::RationalTime> previewPending;
            std::chrono::steady_clock::time_point previewTimer;
            std::shared_ptr<ftk::Observer<std::optional<OTIO_NS::RationalTime> > > timeHoverObserver;
            std::shared_ptr<ftk::Observer<models::WindowSettings> > windowSettingsObserver;
        };

        void MainWindow::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app)
        {
            const models::WindowSettings& settings = app->getSettingsModel()->getWindow();
            Window::_init(
                context,
                app,
                app->getAppInfoModel()->getTitle(),
                settings.size);
            FTK_P();

            // Where it was, and maximized if it was. The size above is the
            // one it had before it was maximized, so that is what it goes
            // back to; opened at its maximized size instead, it came back
            // filling the screen without being maximized, wherever the
            // platform chose to put it.
            if (settings.hasPos)
            {
                setPos(settings.pos);
            }
            if (settings.maximized)
            {
                setMaximized(true);
            }

            ui::initIcons(context);
            auto iconSystem = context->getSystem<ftk::IconSystem>();
            setIcon(iconSystem->get("DJV_Icon", 1.0));

            p.app = app;
            p.settingsModel = app->getSettingsModel();
            p.presentMode = ftk::Observable<bool>::create(false);
            // The presentation hint waits for full screen: on macOS going
            // full screen animates, and a hint shown at the start was
            // stretched with the window for the length of it. Not for ever,
            // though: a window that is refused full screen is still showing
            // nothing but the view.
            p.presentHintTimer = ftk::Timer::create(context);
            setFullScreenCallback(
                [this](bool value)
                {
                    FTK_P();
                    if (value && p.presentHint)
                    {
                        p.presentHint = false;
                        p.presentHintTimer->stop();
                        _presentHint();
                    }
                });

            p.viewport = ui::Viewport::create(
                context,
                app->getFilesModel(),
                app->getColorModel(),
                app->getViewportModel(),
                app->getTimeUnitsModel(),
                app->getSettingsModel(),
                app->getAnnotationsModel(),
                app->getDrawModel(),
                app->getSysLogModel());
            ftk::setScreenshotTag(p.viewport, "MainWindow.Viewport");

            auto timeUnitsModel = app->getTimeUnitsModel();
            p.timelineWidget = tl::ui::TimelineWidget::create(context, timeUnitsModel);
            ftk::setScreenshotTag(p.timelineWidget, "MainWindow.Timeline");

            p.fileActions = FileActions::create(context, app);
            p.reviewActions = ReviewActions::create(
                context,
                app,
                std::dynamic_pointer_cast<MainWindow>(shared_from_this()));
            p.compareActions = CompareActions::create(context, app);
            p.playbackActions = PlaybackActions::create(context, app);
            p.frameActions = FrameActions::create(
                context,
                app,
                std::dynamic_pointer_cast<MainWindow>(shared_from_this()));
            p.timelineActions = TimelineActions::create(
                context,
                app,
                std::dynamic_pointer_cast<MainWindow>(shared_from_this()));
            p.audioActions = AudioActions::create(context, app);
            p.viewActions = ViewActions::create(
                context,
                app,
                std::dynamic_pointer_cast<MainWindow>(shared_from_this()));
            p.windowActions = WindowActions::create(
                context,
                app,
                std::dynamic_pointer_cast<MainWindow>(shared_from_this()));
            p.colorActions = ColorActions::create(context, app);
            p.toolsActions = ToolsActions::create(context, app);
            p.helpActions = HelpActions::create(
                context,
                app,
                std::dynamic_pointer_cast<MainWindow>(shared_from_this()));

            p.fileMenu = FileMenu::create(context, app, p.fileActions);
            p.reviewMenu = ReviewMenu::create(context, app, p.reviewActions);
            p.compareMenu = CompareMenu::create(context, app, p.compareActions);
            p.playbackMenu = PlaybackMenu::create(context, p.playbackActions);
            p.frameMenu = FrameMenu::create(context, p.frameActions);
            p.timelineMenu = TimelineMenu::create(context, p.timelineActions);
            p.audioMenu = AudioMenu::create(context, p.audioActions);
            p.viewMenu = ViewMenu::create(context, app, p.viewActions);
            p.windowMenu = WindowMenu::create(
                context,
                std::dynamic_pointer_cast<MainWindow>(shared_from_this()),
                p.windowActions);
            p.colorMenu = ColorMenu::create(context, p.colorActions);
            p.toolsMenu = ToolsMenu::create(context, app, p.toolsActions);
            p.helpMenu = HelpMenu::create(context, p.helpActions);
            p.menuBar = ftk::MenuBar::create(context);
            ftk::setScreenshotTag(p.menuBar, "MainWindow.MenuBar");
            p.menuBar->addMenu("File", p.fileMenu);
            p.menuBar->addMenu("Review", p.reviewMenu);
            p.menuBar->addMenu("Compare", p.compareMenu);
            p.menuBar->addMenu("Playback", p.playbackMenu);
            p.menuBar->addMenu("Frame", p.frameMenu);
            p.menuBar->addMenu("Timeline", p.timelineMenu);
            p.menuBar->addMenu("Audio", p.audioMenu);
            p.menuBar->addMenu("View", p.viewMenu);
            p.menuBar->addMenu("Window", p.windowMenu);
            p.menuBar->addMenu("Color", p.colorMenu);
            p.menuBar->addMenu("Tools", p.toolsMenu);
            p.menuBar->addMenu("Help", p.helpMenu);

            p.fileToolBar = FileToolBar::create(
                context,
                p.fileActions->getActions());
            ftk::setScreenshotTag(p.fileToolBar, "MainWindow.FileToolBar");

            p.compareToolBar = CompareToolBar::create(
                context,
                p.compareActions->getActions());
            ftk::setScreenshotTag(p.compareToolBar, "MainWindow.CompareToolBar");

            p.viewToolBar = ViewToolBar::create(
                context,
                app,
                std::dynamic_pointer_cast<MainWindow>(shared_from_this()),
                p.viewActions);
            ftk::setScreenshotTag(p.viewToolBar, "MainWindow.ViewToolBar");

            p.windowToolBar = WindowToolBar::create(
                context,
                p.windowActions->getActions());
            ftk::setScreenshotTag(p.windowToolBar, "MainWindow.WindowToolBar");

            p.toolsToolBar = ToolsToolBar::create(
                context,
                app,
                p.toolsActions->getActions());
            ftk::setScreenshotTag(p.toolsToolBar, "MainWindow.ToolsToolBar");

            p.tabBar = TabBar::create(context, app);
            ftk::setScreenshotTag(p.tabBar, "MainWindow.TabBar");

            p.bottomToolBar = BottomToolBar::create(
                context,
                app,
                p.playbackActions,
                p.frameActions,
                p.reviewActions,
                p.audioActions);
            ftk::setScreenshotTag(p.bottomToolBar, "MainWindow.BottomToolBar");

            p.statusBar = StatusBar::create(context, app);
            ftk::setScreenshotTag(p.statusBar, "MainWindow.StatusBar");

            // The highlighted menu item's tooltip shows in the status bar,
            // flattened to the one line there is.
            auto statusBarWeak = std::weak_ptr<StatusBar>(p.statusBar);
            p.menuBar->setCurrentCallback(
                [statusBarWeak](const std::shared_ptr<ftk::Action>& action)
                {
                    if (auto statusBar = statusBarWeak.lock())
                    {
                        std::string text;
                        if (action)
                        {
                            for (const auto& line :
                                ftk::split(action->getTooltip(), '\n'))
                            {
                                text += text.empty() ? line : (" " + line);
                            }
                        }
                        statusBar->setHint(text);
                    }
                });

            p.toolsWidget = ToolsWidget::create(
                context,
                app,
                std::dynamic_pointer_cast<MainWindow>(shared_from_this()));
            ftk::setScreenshotTag(p.toolsWidget, "MainWindow.Tools");

            p.layout = ftk::VerticalLayout::create(context, shared_from_this());
            p.layout->setSpacingRole(ftk::SizeRole::None);
            p.menuBar->setParent(p.layout);
            p.menuBarDivider = ftk::Divider::create(context, ftk::Orientation::Vertical, p.layout);
            p.toolBarLayout = ftk::HorizontalLayout::create(context, p.layout);
            p.toolBarLayout->setMarginRole(ftk::SizeRole::MarginSmall);
            p.toolBarLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            ftk::setScreenshotTag(p.toolBarLayout, "MainWindow.ToolBar");
            // Groups of buttons with a divider between them: the buttons of
            // a group touch, and nothing is drawn behind them but what a
            // button draws for itself, hovered or checked. Space alone
            // either left the groups hard to tell apart or, wide enough to
            // tell, no longer fit a small window.
            bool first = true;
            for (const std::shared_ptr<ftk::ToolBar>& toolBar :
                std::vector<std::shared_ptr<ftk::ToolBar> >{
                    p.fileToolBar,
                    p.compareToolBar,
                    p.windowToolBar,
                    p.viewToolBar,
                    p.toolsToolBar })
            {
                if (!first)
                {
                    ftk::Divider::create(context, ftk::Orientation::Horizontal, p.toolBarLayout);
                }
                first = false;
                toolBar->setSpacingRole(ftk::SizeRole::None);
                toolBar->setParent(p.toolBarLayout);
            }
            p.splitterLayout = ftk::VerticalLayout::create(context, p.layout);
            p.splitterLayout->setSpacingRole(ftk::SizeRole::None);
            p.splitterLayout->setVStretch(ftk::Stretch::Expanding);
            p.splitter = ftk::Splitter::create(context, ftk::Orientation::Vertical, p.splitterLayout);
            p.splitter->setSplit(settings.splitter);
            p.splitter->setBorder(false);
            p.splitter2 = ftk::Splitter::create(context, ftk::Orientation::Horizontal, p.splitter);
            p.splitter2->setSplit(settings.splitter2);
            p.splitter2->setBorder(false);
            auto vLayout = ftk::VerticalLayout::create(context, p.splitter2);
            vLayout->setSpacingRole(ftk::SizeRole::None);
            p.tabBar->setParent(vLayout);
            p.viewport->setParent(vLayout);
            p.toolsWidget->setParent(p.splitter2);
            p.timelineWidget->setParent(p.splitter);
            p.bottomToolBar->setParent(p.layout);
            p.statusBar->setParent(p.layout);

            // Each context menu offers the band it belongs to, and only
            // that band; the Window menu remains the one place that lists
            // every piece of chrome together.
            p.toolBarLayout->setContextMenuCallback(chromeMenuCallback(
                context,
                p.windowActions,
                {
                    "FileToolBar",
                    "CompareToolBar",
                    "WindowToolBar",
                    "ViewToolBar",
                    "ToolsToolBar"
                }));

            // The timeline, playback controls and status bar form one band
            // across the bottom of the window, so right clicking any of
            // them offers the whole band rather than only itself.
            const std::vector<std::string> bottomChrome =
            {
                "Timeline",
                "BottomToolBar",
                "StatusToolBar"
            };
            p.timelineWidget->setContextMenuCallback(
                chromeMenuCallback(context, p.windowActions, bottomChrome));
            p.bottomToolBar->setContextMenuCallback(
                chromeMenuCallback(context, p.windowActions, bottomChrome));
            p.statusBar->setContextMenuCallback(
                chromeMenuCallback(context, p.windowActions, bottomChrome));

            // The viewport, and the tab bar over it, offer every toggle:
            // the viewport can never be hidden, so this is the one door that
            // cannot be closed by hiding chrome. On the window as a whole it
            // also answered a click in the Tools panel, which looked like a
            // menu about the tool rather than the window's.
            auto chromeMenu = chromeMenuCallback(
                context,
                p.windowActions,
                {
                    "FileToolBar",
                    "CompareToolBar",
                    "WindowToolBar",
                    "ViewToolBar",
                    "ToolsToolBar",
                    "TabBar",
                    "Timeline",
                    "BottomToolBar",
                    "StatusToolBar",
                    "Tools"
                });
            const auto viewportMenu =
                [this, chromeMenu]() -> std::shared_ptr<ftk::Menu>
                {
                    // Presentation mode hides every piece of chrome
                    // regardless of these settings, so the toggles would
                    // do nothing you could see. Escape leaves the mode.
                    return _p->presentMode->get() ? nullptr : chromeMenu();
                };
            p.viewport->setContextMenuCallback(viewportMenu);
            p.tabBar->setContextMenuCallback(viewportMenu);

            // The Tools panel is a band of its own.
            p.toolsWidget->setContextMenuCallback(
                chromeMenuCallback(context, p.windowActions, { "Tools" }));

            // Unless the application has something to settle first, which
            // then shows it itself (App::isSetupReady()).
            if (app->isSetupReady())
            {
                showFirstStart();
            }

            p.playerObserver = ftk::Observer<std::shared_ptr<tl::Player> >::create(
                app->observePlayer(),
                [this](const std::shared_ptr<tl::Player>& player)
                {
                    FTK_P();
                    p.player = player;
                    _timelinePreviewClose();
                    p.viewport->setPlayer(player);
                    p.timelineWidget->setPlayer(player);
                    // Whether there is a file decides the timeline layout.
                    if (auto settingsModel = p.settingsModel)
                    {
                        _settingsUpdate(settingsModel->getTimeline());
                    }
                    _windowUpdate();
                });

            // Closing the last file leaves present mode, rather than a black
            // screen with nothing on it to say how to get back (#942).
            p.filesObserver = ftk::ListObserver<std::shared_ptr<models::FilesModelItem> >::create(
                app->getFilesModel()->observeFiles(),
                [this](const std::vector<std::shared_ptr<models::FilesModelItem> >& value)
                {
                    if (value.empty() && _p->presentMode->get())
                    {
                        setPresentMode(false);
                    }
                });

            p.timeHoverObserver = ftk::Observer<std::optional<OTIO_NS::RationalTime> >::create(
                p.timelineWidget->observeTimeHover(),
                [this](const std::optional<OTIO_NS::RationalTime>& value)
                {
                    _timelinePreviewUpdate(value);
                });

            p.compareOptionsObserver = ftk::Observer<tl::CompareOptions>::create(
                p.viewport->observeCompareOptions(),
                [this](const tl::CompareOptions& value)
                {
                    auto app = _p->app.lock();
                    app->getFilesModel()->setCompareOptions(value);
                });

            p.ocioOptionsObserver = ftk::Observer<tl::OCIOOptions>::create(
                app->getColorModel()->observeResolvedOCIOOptions(),
                [this](const tl::OCIOOptions& value)
                {
                    auto options = _p->timelineWidget->getDisplayOptions();
                    options.ocio = value;
                    _p->timelineWidget->setDisplayOptions(options);
                });

            p.lutOptionsObserver = ftk::Observer<tl::LUTOptions>::create(
                app->getColorModel()->observeLUTOptions(),
                [this](const tl::LUTOptions& value)
                {
                    auto options = _p->timelineWidget->getDisplayOptions();
                    options.lut = value;
                    _p->timelineWidget->setDisplayOptions(options);
                });

            p.colorBufferObserver = ftk::Observer<ftk::ImageType>::create(
                app->getViewportModel()->observeColorBuffer(),
                [this](ftk::ImageType value)
                {
                    setBufferType(ftk::ImageType::RGBA_U8 == value ?
                        ftk::WindowBufferType::U8 :
                        ftk::WindowBufferType::F16);
                });

            p.openToolsObserver = ftk::ListObserver<std::string>::create(
                app->getToolsModel()->observeOpenTools(),
                [this](const std::vector<std::string>&)
                {
                    _windowUpdate();
                });

            // A section asked for is one to be looked at, so the tools
            // panel comes back if it was hidden.
            p.showSectionObserver = ftk::Observer<std::pair<std::string, std::string> >::create(
                app->getToolsModel()->observeShowSection(),
                [this](const std::pair<std::string, std::string>&)
                {
                    FTK_P();
                    models::WindowSettings settings = p.settingsModel->getWindow();
                    if (!settings.tools)
                    {
                        settings.tools = true;
                        p.settingsModel->setWindow(settings);
                    }
                },
                ftk::ObserverAction::Suppress);

            p.mouseSettingsObserver = ftk::Observer<models::MouseSettings>::create(
                p.settingsModel->observeMouse(),
                [this](const models::MouseSettings& value)
                {
                    _settingsUpdate(value);
                });

            p.timelineSettingsObserver = ftk::Observer<models::TimelineSettings>::create(
                p.settingsModel->observeTimeline(),
                [this](const models::TimelineSettings& value)
                {
                    _settingsUpdate(value);
                });

            p.timelineFrameViewObserver = ftk::Observer<bool>::create(
                p.timelineWidget->observeFrameView(),
                [this](bool value)
                {
                    auto app = _p->app.lock();
                    auto settings = app->getSettingsModel()->getTimeline();
                    settings.frameView = value;
                    app->getSettingsModel()->setTimeline(settings);
                });

            p.windowSettingsObserver = ftk::Observer<models::WindowSettings>::create(
                p.settingsModel->observeWindow(),
                [this](const models::WindowSettings&)
                {
                    _windowUpdate();
                });
        }

        MainWindow::MainWindow() :
            _p(new Private)
        {}

        MainWindow::~MainWindow()
        {
            FTK_P();

            // Nothing was made when making the window failed; the
            // application lets go of it on the way out all the same.
            if (p.viewport)
            {
                _makeCurrent();
                p.viewport->setParent(nullptr);
                p.viewport.reset();
                p.timelineWidget->setParent(nullptr);
                p.timelineWidget.reset();
            }

            saveSettings();
        }

        void MainWindow::saveSettings()
        {
            FTK_P();
            if (p.shown)
            {
                models::WindowSettings settings = p.settingsModel->getWindow();
                const ftk::Box2I geometry = getNormalGeometry();
                settings.size = geometry.size();
                settings.pos = geometry.min;
                settings.hasPos = true;
                settings.maximized = isMaximized();
                settings.splitter = p.splitter->getSplit();
                settings.splitter2 = p.splitter2->getSplit();
                p.settingsModel->setWindow(settings);
            }
        }

        std::shared_ptr<MainWindow> MainWindow::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app)
        {
            auto out = std::shared_ptr<MainWindow>(new MainWindow);
            out->_init(context, app);
            return out;
        }

        const std::shared_ptr<ftk::MenuBar> MainWindow::getMenuBar() const
        {
            return _p->menuBar;
        }

        void MainWindow::click(const ftk::V2I& pos, int modifiers)
        {
            _cursorEnter(true);
            _cursorPos(pos);
            _mouseButton(ftk::MouseButton::Left, true, modifiers);
            _mouseButton(ftk::MouseButton::Left, false, modifiers);
        }

        const std::shared_ptr<ui::Viewport>& MainWindow::getViewport() const
        {
            return _p->viewport;
        }

        const std::shared_ptr<ReviewActions>& MainWindow::getReviewActions() const
        {
            return _p->reviewActions;
        }

        std::shared_ptr<ftk::Action> MainWindow::getAction(const std::string& value) const
        {
            FTK_P();
            const auto i = value.find('/');
            if (std::string::npos == i)
            {
                return nullptr;
            }
            const std::string group = value.substr(0, i);
            const std::string name = value.substr(i + 1);
            const std::map<std::string, std::shared_ptr<IActions> > groups =
            {
                { "File", p.fileActions },
                { "Review", p.reviewActions },
                { "Compare", p.compareActions },
                { "Playback", p.playbackActions },
                { "Frame", p.frameActions },
                { "Timeline", p.timelineActions },
                { "Audio", p.audioActions },
                { "View", p.viewActions },
                { "Window", p.windowActions },
                { "Color", p.colorActions },
                { "Tools", p.toolsActions },
                { "Help", p.helpActions }
            };
            const auto j = groups.find(group);
            if (j == groups.end() || !j->second)
            {
                return nullptr;
            }
            const auto& actions = j->second->getActions();
            const auto k = actions.find(name);
            return k != actions.end() ? k->second : nullptr;
        }

        const std::shared_ptr<tl::ui::TimelineWidget>& MainWindow::getTimelineWidget() const
        {
            return _p->timelineWidget;
        }

        std::shared_ptr<IToolWidget> MainWindow::getToolWidget(
            const std::string& name) const
        {
            return _p->toolsWidget->getToolWidget(name);
        }

        void MainWindow::setSplitters(float splitter, float splitter2)
        {
            FTK_P();
            if (p.splitter)
            {
                p.splitter->setSplit(splitter);
            }
            if (p.splitter2)
            {
                p.splitter2->setSplit(splitter2);
            }
        }

        bool MainWindow::hasPresentMode() const
        {
            return _p->presentMode->get();
        }

        std::shared_ptr<ftk::IObservable<bool> > MainWindow::observePresentMode() const
        {
            return _p->presentMode;
        }

        void MainWindow::setPresentMode(bool value)
        {
            FTK_P();
            if (p.presentMode->setIfChanged(value))
            {
                const bool wasFullScreen = isFullScreen();
                p.presentHint = value && !wasFullScreen;
                p.presentHintTimer->stop();
                if (p.presentHint)
                {
                    p.presentHintTimer->start(
                        std::chrono::seconds(1),
                        [this]
                        {
                            FTK_P();
                            if (p.presentHint)
                            {
                                p.presentHint = false;
                                _presentHint();
                            }
                        });
                }
                setFullScreen(value);
                _windowUpdate();
                if (value && wasFullScreen)
                {
                    _presentHint();
                }
            }
        }

        void MainWindow::_presentHint()
        {
            // Nothing else on the screen says how to get back, and with no
            // file open the screen is only black (DJV #905).
            _p->viewport->showHint("Press Esc to exit presentation mode");
        }

        void MainWindow::focusCurrentFrame()
        {
            _p->bottomToolBar->focusCurrentFrame();
        }

        void MainWindow::addReviewNote()
        {
            FTK_P();
            if (auto app = p.app.lock())
            {
                // Opening the tool creates it if this is the first time.
                app->getToolsModel()->setToolOpen("Review", true);
            }
            if (auto reviewTool = std::dynamic_pointer_cast<ReviewTool>(
                p.toolsWidget->getToolWidget("Review")))
            {
                reviewTool->addNote();
            }
        }

        void MainWindow::exportMovie(
            bool overwrite,
            const std::function<void(bool)>& callback)
        {
            FTK_P();
            if (auto app = p.app.lock())
            {
                app->getToolsModel()->setToolOpen("Export", true);
            }
            if (auto exportTool = std::dynamic_pointer_cast<ExportTool>(
                p.toolsWidget->getToolWidget("Export")))
            {
                // The export draws with OpenGL in this window's context,
                // which is current once the window has been drawn and not
                // before: a command given at startup can get here first,
                // and nothing can then be made.
                _makeCurrent();
                exportTool->exportMovie(overwrite, callback);
            }
            else if (callback)
            {
                callback(false);
            }
        }

        void MainWindow::addReviewRange()
        {
            FTK_P();
            if (auto app = p.app.lock())
            {
                app->getToolsModel()->setToolOpen("Review", true);
            }
            if (auto reviewTool = std::dynamic_pointer_cast<ReviewTool>(
                p.toolsWidget->getToolWidget("Review")))
            {
                reviewTool->addRange();
            }
        }

        void MainWindow::showFirstStart()
        {
            FTK_P();
            auto app = p.app.lock();
            if (!app || p.tourOverlay)
                return;
            auto settingsModel = app->getSettingsModel();
            auto miscSettings = settingsModel->getMisc();
            if (!miscSettings.showSetup || app->getHideSetup())
                return;
            // Cleared when it is shown, not when it is closed: a first run
            // is shown the tour once, however it ends.
            miscSettings.showSetup = false;
            settingsModel->setMisc(miscSettings);

            // The tour, which opens the sample timeline when nothing was
            // asked for, and ends at the settings a first run should know
            // of.
            _showTour(nullptr);
        }

        void MainWindow::showTour()
        {
            _showTour(nullptr);
        }

        void MainWindow::_showTour(const std::function<void(void)>& done)
        {
            FTK_P();
            auto app = p.app.lock();
            if (!app || p.tourOverlay)
                return;

            // The steps point at the parts of the window by their screenshot
            // tags, so a part turned off in the Window menu is skipped. The
            // controls differ between a desk and a tablet, and so does the
            // text.
            // The sample files, so that the tour has pictures to point at
            // and a newcomer starts with something playing rather than a
            // file browser to learn: a turntable of a train model, shaded
            // and as a wireframe, so that the compare step has two files
            // that differ. When nothing is open, and where the
            // documentation is installed, which is where the samples are
            // (beside the iPad's documentation too).
            auto filesModel = app->getFilesModel();
            if (filesModel->getFiles().empty())
            {
                const std::string shaded = app->getAppInfoModel()->getDocsFile("Train-Shaded.otioz");
                const std::string wireframe = app->getAppInfoModel()->getDocsFile("Train-Wireframe.otioz");
                if (!shaded.empty() && !wireframe.empty())
                {
                    app->open(ftk::Path(shaded));
                    app->open(ftk::Path(wireframe));
                    // The shaded one in front, and the wireframe as B, so
                    // the compare controls have something to compare from
                    // the start; opening makes the last one current.
                    filesModel->setA(0);
                    filesModel->setB(1, true);
                }
            }

            // The compare step puts the two files in a wipe, and the tour
            // puts the comparison back as it was when it ends.
            struct CompareState
            {
                bool changed = false;
                tl::CompareOptions options;
                std::vector<int> bIndexes;
            };
            auto compareState = std::make_shared<CompareState>();

            auto toolsModel = app->getToolsModel();

            // A control listed in a callout is an action's icon and name,
            // so the list says what the toolbar shows.
            const auto item = [this](const std::string& action)
            {
                ui::TourItem out;
                if (auto a = getAction(action))
                {
                    out.icon = a->getIcon();
                    out.text = a->getText();
                }
                return out;
            };
            std::vector<ui::TourItem> toolItems;
            for (const auto& tool : toolsModel->getTools())
            {
                if (tool.toolBar)
                {
                    toolItems.push_back(item("Tools/" + tool.name));
                }
            }
            std::vector<ui::TourStep> steps;

            // A word first, in the middle of the window: what this is and
            // how to leave it.
            steps.push_back(ui::TourStep(
                "",
                ftk::Format("Welcome to {0} {1}").
                    arg(app->getAppInfoModel()->getFullName()).
                    arg(app->getAppInfoModel()->getVersion()),
                "A short tour of the window follows.\n\n"
                "Next goes on, Close skips it, and Tour in the Help menu "
                "shows it again."));
#if defined(__APPLE__) && TARGET_OS_IPHONE
            steps.push_back(ui::TourStep(
                "MainWindow.Viewport",
                "The view",
                "The view shows the current frame.").withNotes({
                    "Pinch: zoom",
                    "Two fingers: pan",
                    "Double tap: frame the view",
                    "Long press: context menu" }));
            steps.push_back(ui::TourStep(
                "MainWindow.FileToolBar",
                "Files",
                "Open, close, and reload files.").withItems({
                    item("File/Open"),
                    item("File/OpenAudio"),
                    item("File/Close"),
                    item("File/CloseAll"),
                    item("File/Reload") }).withNotes({
                    "Open shows the system's picker; Open Folder, in the "
                    "File menu, opens the file browser inside a folder.",
                    "Closed files can be opened again from Recent Files in "
                    "the File menu." }));
#else // TARGET_OS_IPHONE
            steps.push_back(ui::TourStep(
                "MainWindow.Viewport",
                "The view",
                "The view shows the current frame.").withNotes({
                    "Mouse wheel: zoom",
                    "Middle button: pan",
                    "Backspace: frame the view" }));
            steps.push_back(ui::TourStep(
                "MainWindow.FileToolBar",
                "Files",
                "Open, close, and reload files.").withItems({
                    item("File/Open"),
                    item("File/OpenAudio"),
                    item("File/Close"),
                    item("File/CloseAll"),
                    item("File/Reload") }).withNotes({
                    "Files can also be dropped on the window.",
                    "Closed files can be opened again from Recent Files in "
                    "the File menu." }));
#endif // TARGET_OS_IPHONE
            steps.push_back(ui::TourStep(
                "MainWindow.TabBar",
                "Open files",
                "Each open file has a tab, click to switch between them."));
#if defined(__APPLE__) && TARGET_OS_IPHONE
            steps.push_back(ui::TourStep(
                "MainWindow.Timeline",
                "Timeline",
                "Clips, tracks, video thumbnails, and audio waveforms.").withNotes({
                    "Drag: scrub the current frame",
                    "Pinch: zoom",
                    "Two fingers: pan" }));
            steps.push_back(ui::TourStep(
                "Playback.PlaybackControls",
                "Playback",
                "Playback, loop mode, and shuttle.").withItems({
                    item("Playback/Reverse"),
                    item("Playback/Stop"),
                    item("Playback/Forward"),
                    item("Playback/Loop"),
                    item("Playback/Once"),
                    item("Playback/PingPong") }).withNotes({
                    "Forward or reverse again: faster",
                    "Drag the shuttle: vary the speed" }));
            steps.push_back(ui::TourStep(
                "Playback.FrameControls",
                "Frames",
                "Frame controls and shuttle.").withItems({
                    item("Frame/Start"),
                    item("Frame/Prev"),
                    item("Frame/Next"),
                    item("Frame/End") }).withNotes({
                    "Drag the shuttle: scrub the current frame" }));
#else // TARGET_OS_IPHONE
            steps.push_back(ui::TourStep(
                "MainWindow.Timeline",
                "Timeline",
                "Clips, tracks, video thumbnails, and audio waveforms.").withNotes({
                    "Drag: scrub the current frame",
                    "Hover: preview the frame",
                    "Mouse wheel: zoom",
                    "Middle button: pan" }));
            steps.push_back(ui::TourStep(
                "Playback.PlaybackControls",
                "Playback",
                "Playback, loop mode, and shuttle.").withItems({
                    item("Playback/Reverse"),
                    item("Playback/Stop"),
                    item("Playback/Forward"),
                    item("Playback/Loop"),
                    item("Playback/Once"),
                    item("Playback/PingPong") }).withNotes({
                    "Space: toggle playback",
                    "Forward or reverse again: faster",
                    "Drag the shuttle: vary the speed" }));
            steps.push_back(ui::TourStep(
                "Playback.FrameControls",
                "Frames",
                "Frame controls and shuttle.").withItems({
                    item("Frame/Start"),
                    item("Frame/Prev"),
                    item("Frame/Next"),
                    item("Frame/End") }).withNotes({
                    "Arrow keys: step a frame",
                    "Drag the shuttle: scrub the current frame" }));
#endif // TARGET_OS_IPHONE
            steps.push_back(ui::TourStep(
                "Playback.TimeControls",
                "Time",
                "Current frame, duration, time units, and speed controls."));
            steps.push_back(ui::TourStep(
                "MainWindow.ViewToolBar",
                "View tools",
                "Frame and zoom the view.").withItems({
                    item("View/Frame"),
                    item("View/ZoomReset") }));
            steps.push_back(ui::TourStep(
                "MainWindow.CompareToolBar",
                "Compare",
                "Open a second file and choose a compare mode.",
                [filesModel, compareState]
                {
                    // With two or more files open: the second as B, in a
                    // wipe. Remembered once, so that Back and Next do not
                    // remember the tour's own change as the user's.
                    if (filesModel->getFiles().size() < 2)
                        return;
                    if (!compareState->changed)
                    {
                        compareState->changed = true;
                        compareState->options = filesModel->getCompareOptions();
                        compareState->bIndexes = filesModel->getBIndexes();
                    }
                    // B is a file that is not A, whichever A is by now.
                    for (int i : filesModel->getBIndexes())
                    {
                        filesModel->setB(i, false);
                    }
                    filesModel->setB(0 == filesModel->getAIndex() ? 1 : 0, true);
                    tl::CompareOptions options = compareState->options;
                    options.compare = tl::Compare::Wipe;
                    filesModel->setCompareOptions(options);
                }).withItems({
                    item("Compare/B"),
                    item("Compare/Wipe"),
                    item("Compare/Butterfly"),
                    item("Compare/Overlay"),
                    item("Compare/Difference"),
                    item("Compare/Horizontal"),
                    item("Compare/Vertical"),
                    item("Compare/Tile") }).withNotes({
                    "With two files open, the tour puts them in a wipe." }));
            steps.push_back(ui::TourStep(
                "MainWindow.ToolsToolBar",
                "Tools",
                "").withItems(toolItems));

            // The settings a first run should know of, in the Settings
            // tool, where they live: each step opens the tool at its
            // section, and the controls are live under the callout.
            steps.push_back(ui::TourStep(
                "Settings.Cache",
                "Cache settings",
                "Memory for video and audio frames.\n\n"
                "A larger cache plays more from memory.",
                [toolsModel]
                {
                    toolsModel->showSection("Settings", "Cache");
                }).withPlacement(ui::TourPlacement::Left));
            steps.push_back(ui::TourStep(
                "Settings.Style",
                "Style settings",
                "Colors, fonts, and sizes of the user interface.",
                [toolsModel]
                {
                    toolsModel->showSection("Settings", "Style");
                }).withPlacement(ui::TourPlacement::Left));
            steps.push_back(ui::TourStep(
                "Settings.FFmpeg",
                "FFmpeg settings",
                "Hardware decoding, decoder threads, and the FFmpeg programs "
                "used for movies.",
                [toolsModel]
                {
                    toolsModel->showSection("Settings", "FFmpeg");
                }).withPlacement(ui::TourPlacement::Left));
            steps.push_back(ui::TourStep(
                "MainWindow.MenuBar",
                "That is the tour",
#if defined(__APPLE__) && TARGET_OS_IPHONE
                "The menus contain additional controls.\n\n"
#else // TARGET_OS_IPHONE
                "The menus contain additional controls and keyboard "
                "shortcuts.\n\n"
#endif // TARGET_OS_IPHONE
                "Tour, in the Help menu, shows this again, and "
                "Documentation shows the full documentation."));

            // The tools the tour opens, and the comparison it sets, are put
            // back as they were.
            const std::vector<std::string> openTools = toolsModel->getOpenTools();
            p.tourOverlay = ui::TourOverlay::create(getContext(), steps);
            p.tourOverlay->setCloseCallback(
                [this, toolsModel, openTools, filesModel, compareState, done]
                {
                    // Kept until this returns: the callback lives in the
                    // overlay, and resetting the pointer would destroy it
                    // with what it captured.
                    auto overlay = _p->tourOverlay;
                    _p->tourOverlay.reset();
                    toolsModel->closeTools();
                    for (const auto& tool : openTools)
                    {
                        toolsModel->setToolOpen(tool, true);
                    }
                    if (compareState->changed)
                    {
                        for (int i : filesModel->getBIndexes())
                        {
                            filesModel->setB(i, false);
                        }
                        for (int i : compareState->bIndexes)
                        {
                            filesModel->setB(i, true);
                        }
                        filesModel->setCompareOptions(compareState->options);
                    }
                    if (done)
                    {
                        done();
                    }
                });
            p.tourOverlay->open(std::dynamic_pointer_cast<IWindow>(shared_from_this()));
        }

        void MainWindow::showAboutDialog()
        {
            FTK_P();
            p.aboutDialog = ui::AboutDialog::create(
                getContext(),
                p.app.lock()->getAppInfoModel());
            p.aboutDialog->open(std::dynamic_pointer_cast<IWindow>(shared_from_this()));
            p.aboutDialog->setCloseCallback(
                [this]
                {
                    _p->aboutDialog.reset();
                });
        }

        void MainWindow::showSysInfoDialog()
        {
            FTK_P();
            p.sysInfoDialog = ui::SysInfoDialog::create(
                getContext(),
                p.app.lock()->getSysInfo(),
                std::dynamic_pointer_cast<MainWindow>(shared_from_this()));
            p.sysInfoDialog->open(std::dynamic_pointer_cast<IWindow>(shared_from_this()));
            p.sysInfoDialog->setCloseCallback(
                [this]
                {
                    _p->sysInfoDialog.reset();
                });
        }

        void MainWindow::close()
        {
            FTK_P();
            // Intercept the window close (e.g. the title-bar button) to prompt
            // for saving the review before actually closing.
            if (auto app = p.app.lock())
            {
                app->confirmClose(
                    [this]
                    {
                        ftk::Window::close();
                    });
            }
            else
            {
                ftk::Window::close();
            }
        }

        void MainWindow::setGeometry(const ftk::Box2I& value)
        {
            Window::setGeometry(value);
            FTK_P();
            p.shown = true;
            p.layout->setGeometry(value);
        }

        void MainWindow::keyPressEvent(ftk::KeyEvent& event)
        {
            FTK_P();
            if (0 == event.modifiers &&
                ftk::Key::Escape == event.key &&
                p.presentMode->get())
            {
                event.accept = true;
                setPresentMode(false);
            }
            else
            {
                event.accept = p.menuBar->shortcut(event.key, event.modifiers);
            }
        }

        void MainWindow::keyReleaseEvent(ftk::KeyEvent& event)
        {
            event.accept = true;
        }

        void MainWindow::dropEvent(ftk::DragDropEvent& event)
        {
            FTK_P();
            event.accept = true;
            if (auto textData = std::dynamic_pointer_cast<ftk::DragDropTextData>(event.data))
            {
                if (auto app = p.app.lock())
                {
                    // Opened as one change: a file becomes the current one as
                    // it is added, and the current file is the one that gets
                    // read, so a dropped selection would read every file in it
                    // on the way to the last.
                    std::vector<ftk::Path> paths;
                    for (const auto& i : textData->getText())
                    {
                        paths.push_back(ftk::Path(i));
                    }
                    app->open(paths);
                }
            }
        }

        void MainWindow::_settingsUpdate(const models::MouseSettings& settings)
        {
            FTK_P();
            p.timelineWidget->setMouseWheelScale(settings.wheelScale);
        }

        void MainWindow::tickEvent(
            bool parentsVisible,
            bool parentsEnabled,
            const ftk::TickEvent& event)
        {
            Window::tickEvent(parentsVisible, parentsEnabled, event);
            FTK_P();
            if (p.previewPending.has_value() && !p.preview)
            {
                const auto now = std::chrono::steady_clock::now();
                if (now - p.previewTimer > ftk::tooltipTimeout)
                {
                    _timelinePreviewOpen(p.previewPending.value());
                }
            }
        }

        void MainWindow::_timelinePreviewUpdate(const std::optional<OTIO_NS::RationalTime>& value)
        {
            FTK_P();
            // Only while stopped: a preview is a seek through the readers
            // the player is using, and during playback that is a stall for
            // a picture nobody is looking at.
            if (value.has_value() &&
                p.timelinePreview &&
                p.player &&
                tl::Playback::Stop == p.player->getPlayback())
            {
                if (p.preview)
                {
                    _timelinePreviewOpen(value.value());
                }
                else
                {
                    // The delay is the tooltip's, so that a click to seek
                    // and a move off the timeline do not bring it back.
                    if (!p.previewPending.has_value())
                    {
                        p.previewTimer = std::chrono::steady_clock::now();
                    }
                    p.previewPending = value;
                }
            }
            else
            {
                _timelinePreviewClose();
            }
        }

        void MainWindow::_timelinePreviewOpen(const OTIO_NS::RationalTime& value)
        {
            FTK_P();
            if (auto app = p.app.lock())
            {
                if (!p.preview)
                {
                    if (auto context = getContext())
                    {
                        p.preview = ui::TimelinePreview::create(
                            context,
                            p.player,
                            app->getTimeUnitsModel(),
                            std::dynamic_pointer_cast<IWidget>(shared_from_this()));
                    }
                }
                if (p.preview)
                {
                    auto colorModel = app->getColorModel();
                    auto viewportModel = app->getViewportModel();
                    p.preview->setImageOptions(viewportModel->getImageOptions());
                    p.preview->setDisplayOptions(viewportModel->getDisplayOptions());
                    p.preview->setOCIOOptions(colorModel->getOCIOOptions());
                    p.preview->setOCIOInputResolver(
                        [colorModel](const std::string& path, const ftk::ImageTags& tags)
                        {
                            return colorModel->getOCIOOptions().input.empty() ?
                                colorModel->resolveInput(path, tags) :
                                std::string();
                        });
                    p.preview->setLUTOptions(colorModel->getLUTOptions());
                    p.preview->setPos(ftk::V2I(
                        getCursorPos().x,
                        p.timelineWidget->getGeometry().min.y));
                    p.preview->setTime(value);
                }
            }
        }

        void MainWindow::_timelinePreviewClose()
        {
            FTK_P();
            p.previewPending.reset();
            if (p.preview)
            {
                p.preview->close();
                p.preview.reset();
            }
        }

        void MainWindow::_settingsUpdate(const models::TimelineSettings& settings)
        {
            FTK_P();

            p.timelineWidget->setFrameView(settings.frameView);
            p.timelineWidget->setScrollBarsVisible(settings.scrollBars);
            // Unset, the widget draws a timeline for each file being
            // compared, under the current file's; an empty list is the
            // current file's and no other. Off until asked for: a comparison
            // is usually something done to the picture, and the timelines
            // are what is compared when the files are two cuts.
            p.timelineWidget->setTimelines(
                settings.compareTimelines ?
                std::optional<std::vector<std::shared_ptr<tl::Timeline> > >() :
                std::vector<std::shared_ptr<tl::Timeline> >());
            p.timelineWidget->setAutoScroll(settings.autoScroll);
            p.timelineWidget->setStopOnScrub(settings.stopOnScrub);
            p.timelinePreview = settings.preview;
            if (!p.timelinePreview)
            {
                _timelinePreviewClose();
            }

            // With nothing open the timeline is hidden, so it takes the
            // minimized layout to drop the splitter.
            const bool minimize = settings.minimize || !p.player;

            auto display = p.timelineWidget->getDisplayOptions();

            display.minimize = minimize;
            // The color of an edit field rather than the window's: the
            // timeline is where things are worked on, and the ruler above it
            // is already that color.
            display.background = ftk::ColorRole::Base;
            // Track media gates the two rather than replacing them, so that
            // turning it off and on leaves the choice below it alone.
            display.thumbnails = settings.trackMedia && settings.thumbnails;
            display.thumbnailHeight = getTimelineThumbnailSize(settings.thumbnailSize);
            display.waveforms = settings.trackMedia && settings.waveforms;
            display.waveformHeight = getTimelineWaveformSize(settings.waveformSize);
            p.timelineWidget->setDisplayOptions(display);

            if (minimize)
            {
                if (p.splitter->getParent())
                {
                    p.splitter->setParent(nullptr);
                    p.splitter2->setParent(p.splitterLayout);
                    p.timelineWidget->setParent(p.splitterLayout);
                }
            }
            else
            {
                if (!p.splitter->getParent())
                {
                    p.splitter->setParent(p.splitterLayout);
                    p.splitter2->setParent(p.splitter);
                    p.timelineWidget->setParent(p.splitter);
                }
            }
        }

        void MainWindow::_windowUpdate()
        {
            FTK_P();
            if (auto app = p.app.lock())
            {
                auto settings = p.settingsModel->getWindow();
                const bool presentMode = p.presentMode->get();

                p.menuBar->setVisible(!presentMode);
                p.menuBarDivider->setVisible(!presentMode);

                p.fileToolBar->setVisible(settings.fileToolBar && !presentMode);

                p.compareToolBar->setVisible(settings.compareToolBar && !presentMode);

                p.windowToolBar->setVisible(settings.windowToolBar && !presentMode);

                p.viewToolBar->setVisible(settings.viewToolBar && !presentMode);

                p.toolsToolBar->setVisible(settings.toolsToolBar && !presentMode);

                ftk::updateDividers(p.toolBarLayout);

                // The row goes when all of its tool bars do, rather than
                // leaving its margin behind.
                p.toolBarLayout->setVisible(
                    (settings.fileToolBar ||
                    settings.compareToolBar ||
                    settings.windowToolBar ||
                    settings.viewToolBar ||
                    settings.toolsToolBar) && !presentMode);

                p.tabBar->setVisible(settings.tabBar && !presentMode);

                p.toolsWidget->setVisible(
                    settings.tools &&
                    !app->getToolsModel()->getOpenTools().empty() &&
                    !presentMode);

                // With nothing open the timeline has nothing to show.
                p.timelineWidget->setVisible(
                    settings.timeline &&
                    p.player &&
                    !presentMode);

                p.bottomToolBar->setVisible(settings.bottomToolBar && !presentMode);

                p.statusBar->setVisible(settings.statusToolBar && !presentMode);

                // Errors appear over the viewport whether or not there is a
                // status bar: the status bar is where you go to look, and the
                // point of these is that they come to you. Only warnings and
                // errors reach them, so there is nothing routine to interrupt
                // with. Not in presentation mode: an error balloon over someone
                // else's review is worse than a missed message, and the
                // messages tool still has them.
                p.viewport->setToastActive(!presentMode);
                // Hidden rather than turned off, so that what was being shown
                // is still being shown on the way back out.
                p.viewport->setHUDActive(!presentMode);
            }
        }
    }
}
