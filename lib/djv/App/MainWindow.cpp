// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/MainWindow.h>

#include <djv/App/App.h>
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
#include <djv/App/PlaybackMenu.h>
#include <djv/App/PlaybackUIState.h>
#include <djv/App/StatusBar.h>
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
#include <djv/App/Viewport.h>
#include <djv/App/WindowActions.h>
#include <djv/App/WindowMenu.h>
#include <djv/App/WindowToolBar.h>
#include <djv/UI/AboutDialog.h>
#include <djv/UI/SetupDialog.h>
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
#include <ftk/UI/DrawUtil.h>
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

#include <algorithm>
#include <chrono>
#include <cmath>

namespace djv_resource
{
    extern std::vector<uint8_t> DJV_Icon;
}

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

            class PlaybackProgressWidget : public ftk::IWidget
            {
            protected:
                void _init(
                    const std::shared_ptr<ftk::Context>& context,
                    const std::shared_ptr<ftk::IWidget>& parent)
                {
                    IWidget::_init(
                        context,
                        "djv::app::PlaybackProgressWidget",
                        parent);
                    setHStretch(ftk::Stretch::Expanding);
                    setTooltip(
                        "Current position in the complete media duration.");
                }

                PlaybackProgressWidget() = default;

            public:
                static std::shared_ptr<PlaybackProgressWidget> create(
                    const std::shared_ptr<ftk::Context>& context,
                    const std::shared_ptr<ftk::IWidget>& parent = nullptr)
                {
                    auto out = std::shared_ptr<PlaybackProgressWidget>(
                        new PlaybackProgressWidget);
                    out->_init(context, parent);
                    return out;
                }

                void setPlayer(const std::shared_ptr<tl::Player>& player)
                {
                    _currentTimeObserver.reset();
                    _timeRange = player ?
                        player->getTimeRange() :
                        tl::invalidTimeRange;
                    if (player)
                    {
                        _setCurrentTime(player->getCurrentTime());
                        _currentTimeObserver =
                            ftk::Observer<OTIO_NS::RationalTime>::create(
                                player->observeCurrentTime(),
                                [this](const OTIO_NS::RationalTime& value)
                                {
                                    _setCurrentTime(value);
                                });
                    }
                    else
                    {
                        _progress = 0.0;
                        setDrawUpdate();
                    }
                }

                ftk::Size2I getSizeHint() const override
                {
                    return ftk::Size2I(1, _height);
                }

                void sizeHintEvent(
                    const ftk::SizeHintEvent& event) override
                {
                    IWidget::sizeHintEvent(event);
                    _height = std::max(
                        2,
                        static_cast<int>(
                            std::round(2.F * event.displayScale)));
                }

                void drawEvent(
                    const ftk::Box2I& drawRect,
                    const ftk::DrawEvent& event) override
                {
                    IWidget::drawEvent(drawRect, event);
                    const ftk::Box2I& g = getGeometry();
                    event.render->drawRect(
                        g,
                        event.style->getColorRole(ftk::ColorRole::Base));
                    const int width = std::clamp(
                        static_cast<int>(std::round(_progress * g.w())),
                        0,
                        g.w());
                    if (width > 0)
                    {
                        event.render->drawRect(
                            ftk::Box2I(g.x(), g.y(), width, g.h()),
                            event.style->getColorRole(ftk::ColorRole::Red));
                    }
                }

            private:
                void _setCurrentTime(
                    const OTIO_NS::RationalTime& value)
                {
                    const double progress = getPlaybackProgress(
                        value.rescaled_to(1.0).value(),
                        _timeRange.start_time().rescaled_to(1.0).value(),
                        _timeRange.duration().rescaled_to(1.0).value());
                    if (progress != _progress)
                    {
                        _progress = progress;
                        setDrawUpdate();
                    }
                }

                int _height = 2;
                double _progress = 0.0;
                OTIO_NS::TimeRange _timeRange = tl::invalidTimeRange;
                std::shared_ptr<
                    ftk::Observer<OTIO_NS::RationalTime> >
                    _currentTimeObserver;
            };
        }

        struct MainWindow::Private
        {
            std::weak_ptr<App> app;
            std::shared_ptr<models::SettingsModel> settingsModel;
            std::shared_ptr<ftk::Observable<bool> > presentMode;
            bool shown = false;

            std::shared_ptr<Viewport> viewport;
            std::shared_ptr<tl::ui::TimelineWidget> timelineWidget;
            std::shared_ptr<FileActions> fileActions;
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
            std::shared_ptr<FileToolBar> fileToolBar;
            std::shared_ptr<CompareToolBar> compareToolBar;
            std::shared_ptr<ViewToolBar> viewToolBar;
            std::shared_ptr<WindowToolBar> windowToolBar;
            std::shared_ptr<ToolsToolBar> toolsToolBar;
            std::shared_ptr<TabBar> tabBar;
            std::shared_ptr<BottomToolBar> bottomToolBar;
            std::shared_ptr<StatusBar> statusBar;
            std::shared_ptr<PlaybackProgressWidget> playbackProgress;
            std::shared_ptr<ftk::VerticalLayout> playbackBar;
            std::shared_ptr<ftk::HorizontalLayout> playbackRow;
            std::shared_ptr<ftk::VerticalLayout> normalBottomContainer;
            std::shared_ptr<ftk::VerticalLayout> normalStatusContainer;
            std::shared_ptr<ftk::VerticalLayout> overlayBottomContainer;
            std::shared_ptr<ftk::VerticalLayout> overlayStatusContainer;
            std::shared_ptr<ftk::Divider> playbackStatusDivider;
            PlaybackOverlayState playbackOverlayState;
            std::shared_ptr<ftk::Timer> playbackOverlayTimer;
            std::shared_ptr<ToolsWidget> toolsWidget;
            std::shared_ptr<ui::SetupDialog> setupDialog;
            std::shared_ptr<ui::AboutDialog> aboutDialog;
            std::shared_ptr<ui::SysInfoDialog> sysInfoDialog;
            std::map<std::string, std::shared_ptr<ftk::Divider> > dividers;
            std::shared_ptr<ftk::Splitter> splitter;
            std::shared_ptr<ftk::Splitter> splitter2;
            std::shared_ptr<ftk::VerticalLayout> splitterLayout;
            std::shared_ptr<ftk::VerticalLayout> layout;

            std::shared_ptr<ftk::Observer<std::shared_ptr<tl::Player> > > playerObserver;
            std::shared_ptr<ftk::Observer<tl::CompareOptions> > compareOptionsObserver;
            std::shared_ptr<ftk::Observer<tl::OCIOOptions> > ocioOptionsObserver;
            std::shared_ptr<ftk::Observer<tl::LUTOptions> > lutOptionsObserver;
            std::shared_ptr<ftk::Observer<ftk::gl::TextureType> > colorBufferObserver;
            std::shared_ptr<ftk::Observer<std::string> > activeToolObserver;
            std::shared_ptr<ftk::Observer<models::MouseSettings> > mouseSettingsObserver;
            std::shared_ptr<ftk::Observer<models::TimelineSettings> > timelineSettingsObserver;
            std::shared_ptr<ftk::Observer<bool> > timelineFrameViewObserver;
            std::shared_ptr<ftk::Observer<models::WindowSettings> > windowSettingsObserver;
            std::shared_ptr<ftk::Observer<bool> > fullScreenObserver;
        };

        void MainWindow::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app)
        {
            const models::WindowSettings& settings = app->getSettingsModel()->getWindow();
            Window::_init(
                context,
                app,
                ftk::Format("{0} {1}").
                    arg(app->getAppInfoModel()->getFullName()).
                    arg(app->getAppInfoModel()->getVersion()),
                settings.size);
            FTK_P();

            auto iconSystem = context->getSystem<ftk::IconSystem>();
            iconSystem->add("DJV_Icon", djv_resource::DJV_Icon);
            setIcon(iconSystem->get("DJV_Icon", 1.0));

            p.app = app;
            p.settingsModel = app->getSettingsModel();
            p.presentMode = ftk::Observable<bool>::create(false);

            p.viewport = Viewport::create(context, app);
            ftk::setScreenshotTag(p.viewport, "MainWindow.Viewport");

            auto timeUnitsModel = app->getTimeUnitsModel();
            p.timelineWidget = tl::ui::TimelineWidget::create(context, timeUnitsModel);
            ftk::setScreenshotTag(p.timelineWidget, "MainWindow.Timeline");

            p.fileActions = FileActions::create(context, app);
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
                p.audioActions);
            ftk::setScreenshotTag(p.bottomToolBar, "MainWindow.BottomToolBar");

            p.statusBar = app->createStatusBar();
            ftk::setScreenshotTag(p.statusBar, "MainWindow.StatusBar");

            const auto playbackWindowWeak = std::weak_ptr<MainWindow>(
                std::dynamic_pointer_cast<MainWindow>(
                    shared_from_this()));
            p.bottomToolBar->setFullScreenCallback(
                [playbackWindowWeak](bool value)
                {
                    if (auto window = playbackWindowWeak.lock())
                    {
                        window->setFullScreen(value);
                    }
                });
            p.bottomToolBar->setPinCallback(
                [playbackWindowWeak](bool value)
                {
                    if (auto window = playbackWindowWeak.lock())
                    {
                        window->_setPlaybackPinned(value);
                    }
                });
            p.viewport->setFullScreenCallback(
                [playbackWindowWeak]
                {
                    if (auto window = playbackWindowWeak.lock())
                    {
                        if (window->hasPresentMode())
                        {
                            window->setPresentMode(false);
                        }
                        else
                        {
                            window->setFullScreen(
                                !window->isFullScreen());
                        }
                    }
                });
            p.viewport->setMouseActivityCallback(
                [playbackWindowWeak]
                {
                    if (auto window = playbackWindowWeak.lock())
                    {
                        window->_playbackActivity();
                    }
                });
            p.playbackOverlayTimer = ftk::Timer::create(context);
            p.playbackOverlayTimer->setRepeating(true);

            p.toolsWidget = ToolsWidget::create(
                context,
                app,
                std::dynamic_pointer_cast<MainWindow>(shared_from_this()));
            ftk::setScreenshotTag(p.toolsWidget, "MainWindow.Tools");

            p.layout = ftk::VerticalLayout::create(context, shared_from_this());
            p.layout->setSpacingRole(ftk::SizeRole::None);
            p.menuBar->setParent(p.layout);
            p.dividers["MenuBar"] = ftk::Divider::create(context, ftk::Orientation::Vertical, p.layout);
            auto hLayout = ftk::HorizontalLayout::create(context, p.layout);
            hLayout->setSpacingRole(ftk::SizeRole::Spacing);
            ftk::setScreenshotTag(hLayout, "MainWindow.ToolBar");
            p.fileToolBar->setParent(hLayout);
            p.dividers["File"] = ftk::Divider::create(context, ftk::Orientation::Horizontal, hLayout);
            p.compareToolBar->setParent(hLayout);
            p.dividers["Compare"] = ftk::Divider::create(context, ftk::Orientation::Horizontal, hLayout);
            p.windowToolBar->setParent(hLayout);
            p.dividers["Window"] = ftk::Divider::create(context, ftk::Orientation::Horizontal, hLayout);
            p.viewToolBar->setParent(hLayout);
            p.dividers["View"] = ftk::Divider::create(context, ftk::Orientation::Horizontal, hLayout);
            p.toolsToolBar->setParent(hLayout);
            p.dividers["ToolBars"] = ftk::Divider::create(context, ftk::Orientation::Vertical, p.layout);
            p.splitterLayout = ftk::VerticalLayout::create(context, p.layout);
            p.splitterLayout->setSpacingRole(ftk::SizeRole::None);
            p.splitterLayout->setVStretch(ftk::Stretch::Expanding);
            p.splitter = ftk::Splitter::create(context, ftk::Orientation::Vertical, p.splitterLayout);
            p.splitter->setSplit(settings.splitter);
            p.splitter2 = ftk::Splitter::create(context, ftk::Orientation::Horizontal, p.splitter);
            p.splitter2->setSplit(settings.splitter2);
            auto vLayout = ftk::VerticalLayout::create(context, p.splitter2);
            vLayout->setSpacingRole(ftk::SizeRole::None);
            p.tabBar->setParent(vLayout);
            p.viewport->setParent(vLayout);
            p.toolsWidget->setParent(p.splitter2);
            p.timelineWidget->setParent(p.splitter);
            p.dividers["Bottom"] = ftk::Divider::create(context, ftk::Orientation::Vertical, p.layout);
            p.normalBottomContainer = ftk::VerticalLayout::create(
                context,
                p.layout);
            p.normalBottomContainer->setSpacingRole(ftk::SizeRole::None);
            p.bottomToolBar->setParent(p.normalBottomContainer);
            p.dividers["Status"] = ftk::Divider::create(context, ftk::Orientation::Vertical, p.layout);
            p.normalStatusContainer = ftk::VerticalLayout::create(
                context,
                p.layout);
            p.normalStatusContainer->setSpacingRole(ftk::SizeRole::None);
            p.statusBar->setParent(p.normalStatusContainer);

            p.playbackBar = ftk::VerticalLayout::create(
                context);
            p.playbackBar->setSpacingRole(ftk::SizeRole::None);
            p.playbackBar->setBackgroundRole(ftk::ColorRole::Window);
            ftk::setScreenshotTag(
                p.playbackBar,
                "MainWindow.PlaybackBar");

            p.playbackProgress = PlaybackProgressWidget::create(
                context,
                p.playbackBar);
            p.playbackProgress->setVisible(false);
            ftk::setScreenshotTag(
                p.playbackProgress,
                "Playback.FullScreenProgress");

            p.playbackRow = ftk::HorizontalLayout::create(
                context,
                p.playbackBar);
            p.playbackRow->setSpacingRole(ftk::SizeRole::None);
            ftk::setScreenshotTag(
                p.playbackRow,
                "MainWindow.FullScreenPlaybackRow");
            p.overlayBottomContainer = ftk::VerticalLayout::create(
                context,
                p.playbackRow);
            p.overlayBottomContainer->setSpacingRole(ftk::SizeRole::None);
            p.overlayBottomContainer->setHStretch(ftk::Stretch::Expanding);
            p.playbackStatusDivider = ftk::Divider::create(
                context,
                ftk::Orientation::Horizontal,
                p.playbackRow);
            p.overlayStatusContainer = ftk::VerticalLayout::create(
                context,
                p.playbackRow);
            p.overlayStatusContainer->setSpacingRole(ftk::SizeRole::None);

            // Each context menu offers the band it belongs to, and only
            // that band; the Window menu remains the one place that lists
            // every piece of chrome together.
            hLayout->setContextMenuCallback(chromeMenuCallback(
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

            // Anywhere the click is not claimed, offer every toggle. The
            // window itself is consulted last, so a band keeps its own
            // menu; and because the viewport can never be hidden, this is
            // the one door that cannot be closed by hiding chrome.
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
                    "StatusToolBar"
                });
            setContextMenuCallback(
                [this, chromeMenu]() -> std::shared_ptr<ftk::Menu>
                {
                    // Presentation mode hides every piece of chrome
                    // regardless of these settings, so the toggles would
                    // do nothing you could see. Escape leaves the mode.
                    return _p->presentMode->get() ? nullptr : chromeMenu();
                });

            auto miscSettings = app->getSettingsModel()->getMisc();
            if (miscSettings.showSetup && !app->getHideSetup())
            {
                miscSettings.showSetup = false;
                auto settingsModel = app->getSettingsModel();
                settingsModel->setMisc(miscSettings);
                p.setupDialog = ui::SetupDialog::create(
                    context,
                    app->getAppInfoModel(),
                    settingsModel,
                    app->getTimeUnitsModel());
                p.setupDialog->open(std::dynamic_pointer_cast<IWindow>(shared_from_this()));
                p.setupDialog->setCloseCallback(
                    [this]
                    {
                        _p->setupDialog.reset();
                    });
            }

            p.playerObserver = ftk::Observer<std::shared_ptr<tl::Player> >::create(
                app->observePlayer(),
                [this](const std::shared_ptr<tl::Player>& player)
                {
                    FTK_P();
                    p.viewport->setPlayer(player);
                    p.timelineWidget->setPlayer(player);
                    p.playbackProgress->setPlayer(player);
                });

            p.compareOptionsObserver = ftk::Observer<tl::CompareOptions>::create(
                p.viewport->observeCompareOptions(),
                [this](const tl::CompareOptions& value)
                {
                    auto app = _p->app.lock();
                    app->getFilesModel()->setCompareOptions(value);
                });

            p.ocioOptionsObserver = ftk::Observer<tl::OCIOOptions>::create(
                app->getColorModel()->observeOCIOOptions(),
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

            p.colorBufferObserver = ftk::Observer<ftk::gl::TextureType>::create(
                app->getViewportModel()->observeColorBuffer(),
                [this](ftk::gl::TextureType value)
                {
                    setBufferType(ftk::gl::TextureType::RGBA_U8 == value ?
                        ftk::WindowBufferType::U8 :
                        ftk::WindowBufferType::F16);
                });

            p.activeToolObserver = ftk::Observer<std::string>::create(
                app->getToolsModel()->observeActiveTool(),
                [this](const std::string&)
                {
                    _windowUpdate();
                });

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

            p.fullScreenObserver = ftk::Observer<bool>::create(
                observeFullScreen(),
                [this](bool value)
                {
                    _fullScreenUpdate(value);
                });
        }

        MainWindow::MainWindow() :
            _p(new Private)
        {}

        MainWindow::~MainWindow()
        {
            FTK_P();

            p.playbackOverlayTimer->stop();
            _makeCurrent();
            p.viewport->setParent(nullptr);
            p.viewport.reset();
            p.timelineWidget->setParent(nullptr);
            p.timelineWidget.reset();

            if (p.shown)
            {
                models::WindowSettings settings = p.settingsModel->getWindow();
                settings.size = getSize();
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

        const std::shared_ptr<Viewport>& MainWindow::getViewport() const
        {
            return _p->viewport;
        }

        const std::shared_ptr<tl::ui::TimelineWidget>& MainWindow::getTimelineWidget() const
        {
            return _p->timelineWidget;
        }

        const std::shared_ptr<IToolWidget>& MainWindow::getToolWidget() const
        {
            return _p->toolsWidget->getToolWidget();
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
                if (value)
                {
                    auto app = p.app.lock();
                    auto options = app->getViewportModel()->getHUDOptions();
                    options.enabled = false;
                    app->getViewportModel()->setHUDOptions(options);
                }
                setFullScreen(value);
                _fullScreenUpdate(isFullScreen());
                _windowUpdate();
            }
        }

        void MainWindow::focusCurrentFrame()
        {
            _p->bottomToolBar->focusCurrentFrame();
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

        void MainWindow::setGeometry(const ftk::Box2I& value)
        {
            Window::setGeometry(value);
            FTK_P();
            p.shown = true;
            p.layout->setGeometry(value);
            if (p.playbackOverlayState.isFullScreen())
            {
                _playbackOverlayUpdate();
            }
        }

        void MainWindow::keyPressEvent(ftk::KeyEvent& event)
        {
            FTK_P();
            if (0 == event.modifiers &&
                ftk::Key::Escape == event.key)
            {
                if (p.presentMode->get())
                {
                    event.accept = true;
                    setPresentMode(false);
                }
                else if (isFullScreen())
                {
                    event.accept = true;
                    setFullScreen(false);
                }
                else
                {
                    event.accept =
                        p.menuBar->shortcut(
                            event.key,
                            event.modifiers);
                }
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
                    for (const auto& i : textData->getText())
                    {
                        app->open(ftk::Path(i));
                    }
                }
            }
        }

        void MainWindow::_settingsUpdate(const models::MouseSettings& settings)
        {
            FTK_P();
            p.timelineWidget->setMouseWheelScale(settings.wheelScale);
            p.viewport->setMouseWheelScale(settings.wheelScale);
        }

        void MainWindow::_settingsUpdate(const models::TimelineSettings& settings)
        {
            FTK_P();

            p.timelineWidget->setFrameView(settings.frameView);
            p.timelineWidget->setScrollBarsVisible(settings.scrollBars);
            p.timelineWidget->setAutoScroll(settings.autoScroll);
            p.timelineWidget->setStopOnScrub(settings.stopOnScrub);

            auto display = p.timelineWidget->getDisplayOptions();

            display.minimize = settings.minimize;
            display.thumbnails = settings.thumbnails;
            display.thumbnailHeight = getTimelineThumbnailSize(settings.thumbnailSize);
            display.waveforms = settings.waveforms;
            display.waveformHeight = getTimelineWaveformSize(settings.waveformSize);
            p.timelineWidget->setDisplayOptions(display);

            if (settings.minimize)
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

        void MainWindow::_fullScreenUpdate(bool value)
        {
            FTK_P();
            const bool playbackFullScreen =
                value && !p.presentMode->get();
            const auto now = PlaybackOverlayState::Clock::now();
            p.playbackOverlayState.setFullScreen(
                playbackFullScreen,
                now);
            p.bottomToolBar->setFullScreen(playbackFullScreen);
            p.playbackProgress->setVisible(playbackFullScreen);

            if (playbackFullScreen)
            {
                if (p.bottomToolBar->getParent() !=
                    p.overlayBottomContainer)
                {
                    p.bottomToolBar->setParent(
                        p.overlayBottomContainer);
                }
                if (p.statusBar->getParent() !=
                    p.overlayStatusContainer)
                {
                    p.statusBar->setParent(
                        p.overlayStatusContainer);
                }
                if (p.playbackBar->getParent() !=
                    std::dynamic_pointer_cast<ftk::IWidget>(
                        shared_from_this()))
                {
                    p.playbackBar->setParent(shared_from_this());
                }
                p.playbackOverlayTimer->start(
                    std::chrono::milliseconds(16),
                    [this](
                        const std::chrono::steady_clock::time_point& now,
                        const std::chrono::microseconds&)
                    {
                        _p->playbackOverlayState.tick(now);
                        _playbackOverlayUpdate();
                    });
            }
            else
            {
                p.playbackOverlayTimer->stop();
                if (p.bottomToolBar->getParent() !=
                    p.normalBottomContainer)
                {
                    p.bottomToolBar->setParent(
                        p.normalBottomContainer);
                }
                if (p.statusBar->getParent() !=
                    p.normalStatusContainer)
                {
                    p.statusBar->setParent(
                        p.normalStatusContainer);
                }
                if (p.playbackBar->getParent())
                {
                    p.playbackBar->setParent(nullptr);
                }
            }

            _windowUpdate();
            setGeometry(getGeometry());
        }

        void MainWindow::_playbackActivity()
        {
            FTK_P();
            if (p.playbackOverlayState.isFullScreen())
            {
                p.playbackOverlayState.activity(
                    PlaybackOverlayState::Clock::now());
                p.playbackBar->setVisible(true);
                _playbackOverlayUpdate();
            }
        }

        void MainWindow::_playbackOverlayUpdate()
        {
            FTK_P();
            if (!p.playbackOverlayState.isFullScreen())
            {
                return;
            }

            const auto now = PlaybackOverlayState::Clock::now();
            const double visibility =
                p.playbackOverlayState.getVisibility();
            const bool visible =
                visibility > .0001 ||
                p.playbackOverlayState.wantsVisible(now);
            p.playbackBar->setVisible(visible);
            if (visible)
            {
                const ftk::Box2I& g = getGeometry();
                const int height = std::max(
                    1,
                    p.playbackBar->getSizeHint().h);
                const int y =
                    g.y() +
                    g.h() -
                    static_cast<int>(
                        std::round(height * visibility));
                p.playbackBar->setGeometry(
                    ftk::Box2I(
                        g.x(),
                        y,
                        g.w(),
                        height));
                moveToFront(p.playbackBar);
            }
        }

        void MainWindow::_setPlaybackPinned(bool value)
        {
            FTK_P();
            const auto now = PlaybackOverlayState::Clock::now();
            p.playbackOverlayState.setPinned(value, now);
            p.bottomToolBar->setPinned(
                p.playbackOverlayState.isPinned());
            _playbackActivity();
        }

        void MainWindow::_windowUpdate()
        {
            FTK_P();
            if (auto app = p.app.lock())
            {
                auto settings = p.settingsModel->getWindow();
                const bool presentMode = p.presentMode->get();
                const bool playbackFullScreen =
                    isFullScreen() && !presentMode;
                const bool focusMode =
                    presentMode || playbackFullScreen;

                p.menuBar->setVisible(!focusMode);
                p.dividers["MenuBar"]->setVisible(!focusMode);

                p.fileToolBar->setVisible(settings.fileToolBar && !focusMode);
                p.dividers["File"]->setVisible(settings.fileToolBar && !focusMode);

                p.compareToolBar->setVisible(settings.compareToolBar && !focusMode);
                p.dividers["Compare"]->setVisible(settings.compareToolBar && !focusMode);

                p.windowToolBar->setVisible(settings.windowToolBar && !focusMode);
                p.dividers["Window"]->setVisible(settings.windowToolBar && !focusMode);

                p.viewToolBar->setVisible(settings.viewToolBar && !focusMode);
                p.dividers["View"]->setVisible(settings.viewToolBar && !focusMode);

                p.toolsToolBar->setVisible(settings.toolsToolBar && !focusMode);

                p.dividers["ToolBars"]->setVisible(
                    (settings.fileToolBar ||
                    settings.compareToolBar ||
                    settings.windowToolBar ||
                    settings.viewToolBar ||
                    settings.toolsToolBar) && !focusMode);

                p.tabBar->setVisible(settings.tabBar && !focusMode);

                p.toolsWidget->setVisible(
                    !app->getToolsModel()->getActiveTool().empty() &&
                    !focusMode);

                p.timelineWidget->setVisible(settings.timeline && !focusMode);

                const bool bottomVisible =
                    (settings.bottomToolBar || playbackFullScreen) &&
                    !presentMode;
                const bool statusVisible =
                    (settings.statusToolBar || playbackFullScreen) &&
                    !presentMode;
                p.bottomToolBar->setVisible(bottomVisible);
                p.statusBar->setVisible(statusVisible);
                p.playbackStatusDivider->setVisible(
                    playbackFullScreen &&
                    bottomVisible &&
                    statusVisible);
                p.playbackProgress->setVisible(playbackFullScreen);
                p.bottomToolBar->setFullScreen(playbackFullScreen);

                if (playbackFullScreen)
                {
                    const auto now =
                        PlaybackOverlayState::Clock::now();
                    p.playbackBar->setVisible(
                        p.playbackOverlayState.getVisibility() > .0001 ||
                        p.playbackOverlayState.wantsVisible(now));
                    p.dividers["Bottom"]->setVisible(false);
                    p.dividers["Status"]->setVisible(false);
                    p.normalBottomContainer->setVisible(false);
                    p.normalStatusContainer->setVisible(false);
                }
                else
                {
                    p.playbackBar->setVisible(false);
                    p.normalBottomContainer->setVisible(bottomVisible);
                    p.normalStatusContainer->setVisible(statusVisible);
                    p.dividers["Bottom"]->setVisible(bottomVisible);
                    p.dividers["Status"]->setVisible(statusVisible);
                }
            }
        }
    }
}
