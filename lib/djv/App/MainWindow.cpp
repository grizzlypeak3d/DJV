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
#if defined(TLRENDER_BMD)
#include <tlRender/Device/BMDOutputDevice.h>
#endif // TLRENDER_BMD
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

namespace djv_resource
{
    extern std::vector<uint8_t> DJV_Icon;
}

namespace djv
{
    namespace app
    {
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
            std::shared_ptr<ftk::Observer<models::Tool> > activeToolObserver;
            std::shared_ptr<ftk::Observer<models::MouseSettings> > mouseSettingsObserver;
            std::shared_ptr<ftk::Observer<models::TimelineSettings> > timelineSettingsObserver;
            std::shared_ptr<ftk::Observer<bool> > timelineFrameViewObserver;
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
            p.toolsMenu = ToolsMenu::create(context, p.toolsActions);
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

            p.statusBar = StatusBar::create(context, app);
            ftk::setScreenshotTag(p.statusBar, "MainWindow.StatusBar");

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
            hLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
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
            p.bottomToolBar->setParent(p.layout);
            p.dividers["Status"] = ftk::Divider::create(context, ftk::Orientation::Vertical, p.layout);
            p.statusBar->setParent(p.layout);

            auto miscSettings = app->getSettingsModel()->getMisc();
            if (miscSettings.showSetup)
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
                        ftk::WindowBufferType::F32);
                });

            p.activeToolObserver = ftk::Observer<models::Tool>::create(
                app->getToolsModel()->observeActiveTool(),
                [this](models::Tool)
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
        }

        MainWindow::MainWindow() :
            _p(new Private)
        {}

        MainWindow::~MainWindow()
        {
            FTK_P();

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
            display.waveformHeight = getTimelineWaveformSize(settings.thumbnailSize);
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

        void MainWindow::_windowUpdate()
        {
            FTK_P();
            if (auto app = p.app.lock())
            {
                auto settings = p.settingsModel->getWindow();
                const bool presentMode = p.presentMode->get();

                p.menuBar->setVisible(!presentMode);
                p.dividers["MenuBar"]->setVisible(!presentMode);

                p.fileToolBar->setVisible(settings.fileToolBar && !presentMode);
                p.dividers["File"]->setVisible(settings.fileToolBar && !presentMode);

                p.compareToolBar->setVisible(settings.compareToolBar && !presentMode);
                p.dividers["Compare"]->setVisible(settings.compareToolBar && !presentMode);

                p.windowToolBar->setVisible(settings.windowToolBar && !presentMode);
                p.dividers["Window"]->setVisible(settings.windowToolBar && !presentMode);

                p.viewToolBar->setVisible(settings.viewToolBar && !presentMode);
                p.dividers["View"]->setVisible(settings.viewToolBar && !presentMode);

                p.toolsToolBar->setVisible(settings.toolsToolBar && !presentMode);

                p.dividers["ToolBars"]->setVisible(
                    (settings.fileToolBar ||
                    settings.compareToolBar ||
                    settings.windowToolBar ||
                    settings.viewToolBar ||
                    settings.toolsToolBar) && !presentMode);

                p.tabBar->setVisible(settings.tabBar && !presentMode);

                p.toolsWidget->setVisible(
                    app->getToolsModel()->getActiveTool() != models::Tool::None &&
                    !presentMode);

                p.timelineWidget->setVisible(settings.timeline && !presentMode);

                p.bottomToolBar->setVisible(settings.bottomToolBar && !presentMode);
                p.dividers["Bottom"]->setVisible(settings.bottomToolBar && !presentMode);

                p.statusBar->setVisible(settings.statusToolBar && !presentMode);
                p.dividers["Status"]->setVisible(settings.statusToolBar && !presentMode);
            }
        }
    }
}
