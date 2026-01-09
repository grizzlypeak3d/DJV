// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djvApp/Actions/ViewActions.h>

#include <djvApp/Models/ViewportModel.h>
#include <djvApp/Widgets/Viewport.h>
#include <djvApp/App.h>
#include <djvApp/MainWindow.h>

#include <sstream>

namespace djv
{
    namespace app
    {
        struct ViewActions::Private
        {
            std::shared_ptr<ftk::Observer<bool> > frameViewObserver;
            std::shared_ptr<ftk::Observer<tl::DisplayOptions> > displayOptionsObserver;
            std::shared_ptr<ftk::Observer<tl::ForegroundOptions> > fgOptionsObserver;
            std::shared_ptr<ftk::Observer<bool> > hudObserver;
        };

        void ViewActions::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<MainWindow>& mainWindow)
        {
            IActions::_init(context, app, "View");
            FTK_P();

            auto mainWindowWeak = std::weak_ptr<MainWindow>(mainWindow);
            _actions["Frame"] = ftk::Action::create(
                "Frame",
                "ViewFrame",
                [mainWindowWeak](bool value)
                {
                    if (auto mainWindow = mainWindowWeak.lock())
                    {
                        mainWindow->getViewport()->setFrameView(value);
                    }
                });

            _actions["ZoomReset"] = ftk::Action::create(
                "Zoom Reset",
                "ViewZoomReset",
                [mainWindowWeak]
                {
                    if (auto mainWindow = mainWindowWeak.lock())
                    {
                        mainWindow->getViewport()->viewZoomReset();
                    }
                });

            _actions["ZoomIn"] = ftk::Action::create(
                "Zoom In",
                "ViewZoomIn",
                [mainWindowWeak]
                {
                    if (auto mainWindow = mainWindowWeak.lock())
                    {
                        mainWindow->getViewport()->viewZoomIn();
                    }
                });

            _actions["ZoomOut"] = ftk::Action::create(
                "Zoom Out",
                "ViewZoomOut",
                [mainWindowWeak]
                {
                    if (auto mainWindow = mainWindowWeak.lock())
                    {
                        mainWindow->getViewport()->viewZoomOut();
                    }
                });

            auto appWeak = std::weak_ptr<App>(app);
            _actions["Red"] = ftk::Action::create(
                "Red Channel",
                [appWeak](bool value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getViewportModel()->getDisplayOptions();
                        options.channels = value ?
                            ftk::ChannelDisplay::Red :
                            ftk::ChannelDisplay::Color;
                        app->getViewportModel()->setDisplayOptions(options);
                    }
                });

            _actions["Green"] = ftk::Action::create(
                "Green Channel",
                [appWeak](bool value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getViewportModel()->getDisplayOptions();
                        options.channels = value ?
                            ftk::ChannelDisplay::Green :
                            ftk::ChannelDisplay::Color;
                        app->getViewportModel()->setDisplayOptions(options);
                    }
                });

            _actions["Blue"] = ftk::Action::create(
                "Blue Channel",
                [appWeak](bool value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getViewportModel()->getDisplayOptions();
                        options.channels = value ?
                            ftk::ChannelDisplay::Blue :
                            ftk::ChannelDisplay::Color;
                        app->getViewportModel()->setDisplayOptions(options);
                    }
                });

            _actions["Alpha"] = ftk::Action::create(
                "Alpha Channel",
                [appWeak](bool value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getViewportModel()->getDisplayOptions();
                        options.channels = value ?
                            ftk::ChannelDisplay::Alpha :
                            ftk::ChannelDisplay::Color;
                        app->getViewportModel()->setDisplayOptions(options);
                    }
                });

            _actions["MirrorHorizontal"] = ftk::Action::create(
                "Mirror Horizontal",
                [appWeak](bool value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getViewportModel()->getDisplayOptions();
                        options.mirror.x = value;
                        app->getViewportModel()->setDisplayOptions(options);
                    }
                });

            _actions["MirrorVertical"] = ftk::Action::create(
                "Mirror Vertical",
                [appWeak](bool value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getViewportModel()->getDisplayOptions();
                        options.mirror.y = value;
                        app->getViewportModel()->setDisplayOptions(options);
                    }
                });

            _actions["Grid"] = ftk::Action::create(
                "Grid",
                [appWeak](bool value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getViewportModel()->getForegroundOptions();
                        options.grid.enabled = value;
                        app->getViewportModel()->setForegroundOptions(options);
                    }
                });

            _actions["HUD"] = ftk::Action::create(
                "HUD / Information Display",
                [appWeak](bool value)
                {
                    if (auto app = appWeak.lock())
                    {
                        app->getViewportModel()->setHUD(value);
                    }
                });

            _tooltips =
            {
                { "Frame",  "Frame the view to fit the window." },
                { "ZoomReset", "Reset the view zoom to 1:1." },
                { "ZoomIn", "Zoom the view in." },
                { "ZoomOut", "Zoom the view out." },
                { "Grid", "Toggle the grid." },
                { "HUD", "Toggle the HUD / information display." }
            };

            _shortcutsUpdate(app->getSettingsModel()->getShortcuts());

            p.frameViewObserver = ftk::Observer<bool>::create(
                mainWindow->getViewport()->observeFrameView(),
                [this](bool value)
                {
                    _actions["Frame"]->setChecked(value);
                });

            p.displayOptionsObserver = ftk::Observer<tl::DisplayOptions>::create(
                app->getViewportModel()->observeDisplayOptions(),
                [this](const tl::DisplayOptions& value)
                {
                    _actions["Red"]->setChecked(ftk::ChannelDisplay::Red == value.channels);
                    _actions["Green"]->setChecked(ftk::ChannelDisplay::Green == value.channels);
                    _actions["Blue"]->setChecked(ftk::ChannelDisplay::Blue == value.channels);
                    _actions["Alpha"]->setChecked(ftk::ChannelDisplay::Alpha == value.channels);

                    _actions["MirrorHorizontal"]->setChecked(value.mirror.x);
                    _actions["MirrorVertical"]->setChecked(value.mirror.y);
                });

            p.fgOptionsObserver = ftk::Observer<tl::ForegroundOptions>::create(
                app->getViewportModel()->observeForegroundOptions(),
                [this](const tl::ForegroundOptions& value)
                {
                    _actions["Grid"]->setChecked(value.grid.enabled);
                });

            p.hudObserver = ftk::Observer<bool>::create(
                app->getViewportModel()->observeHUD(),
                [this](bool value)
                {
                    _actions["HUD"]->setChecked(value);
                });
        }

        ViewActions::ViewActions() :
            _p(new Private)
        {}

        ViewActions::~ViewActions()
        {}

        std::shared_ptr<ViewActions> ViewActions::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<MainWindow>& mainWindow)
        {
            auto out = std::shared_ptr<ViewActions>(new ViewActions);
            out->_init(context, app, mainWindow);
            return out;
        }
    }
}
