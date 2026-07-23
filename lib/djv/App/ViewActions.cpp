// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/ViewActions.h>

#include <djv/App/App.h>
#include <djv/App/MainWindow.h>
#include <djv/App/Viewport.h>
#include <djv/Models/ViewportModel.h>

#include <ftk/Core/Format.h>

#include <sstream>

namespace djv
{
    namespace app
    {
        struct ViewActions::Private
        {
            std::shared_ptr<ftk::Observer<bool> > frameViewObserver;
            std::shared_ptr<ftk::Observer<tl::DisplayOptions> > displayOptionsObserver;
            std::shared_ptr<ftk::Observer<models::AspectRatioOptions> > aspectRatioOptionsObserver;
            std::shared_ptr<ftk::Observer<tl::BackgroundOptions> > bgOptionsObserver;
            std::shared_ptr<ftk::Observer<tl::ForegroundOptions> > fgOptionsObserver;
            std::shared_ptr<ftk::Observer<models::HUDOptions> > hudOptionsObserver;
        };

        void ViewActions::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<MainWindow>& mainWindow)
        {
            IActions::_init(context, app, "View");
            FTK_P();

            auto mainWindowWeak = std::weak_ptr<MainWindow>(mainWindow);
            auto appWeak = std::weak_ptr<App>(app);

            // Register the commands.
            _addCheckCommand(
                "Frame",
                "Frame the view to fit the image.",
                [mainWindowWeak](const nlohmann::json& args)
                {
                    const bool value = args.at("value").get<bool>();
                    if (auto mainWindow = mainWindowWeak.lock())
                    {
                        mainWindow->getViewport()->setFrameView(value);
                    }
                });

            _addCommand(
                "ZoomReset",
                "Reset the view zoom to 1:1.",
                [mainWindowWeak](const nlohmann::json&)
                {
                    if (auto mainWindow = mainWindowWeak.lock())
                    {
                        mainWindow->getViewport()->resetZoom();
                    }
                });

            _addCommand(
                "ZoomIn",
                "Zoom the view in.",
                [mainWindowWeak](const nlohmann::json&)
                {
                    if (auto mainWindow = mainWindowWeak.lock())
                    {
                        mainWindow->getViewport()->zoomIn();
                    }
                });

            _addCommand(
                "ZoomOut",
                "Zoom the view out.",
                [mainWindowWeak](const nlohmann::json&)
                {
                    if (auto mainWindow = mainWindowWeak.lock())
                    {
                        mainWindow->getViewport()->zoomOut();
                    }
                });

            _addCommand(
                "Center",
                "Center the view.",
                [mainWindowWeak](const nlohmann::json&)
                {
                    if (auto mainWindow = mainWindowWeak.lock())
                    {
                        mainWindow->getViewport()->center();
                    }
                });

            _addCheckCommand(
                "Red",
                "Show the red channel.",
                [appWeak](const nlohmann::json& args)
                {
                    const bool value = args.at("value").get<bool>();
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getViewportModel()->getDisplayOptions();
                        options.channels = value ?
                            ftk::ChannelDisplay::Red :
                            ftk::ChannelDisplay::Color;
                        app->getViewportModel()->setDisplayOptions(options);
                    }
                });

            _addCheckCommand(
                "Green",
                "Show the green channel.",
                [appWeak](const nlohmann::json& args)
                {
                    const bool value = args.at("value").get<bool>();
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getViewportModel()->getDisplayOptions();
                        options.channels = value ?
                            ftk::ChannelDisplay::Green :
                            ftk::ChannelDisplay::Color;
                        app->getViewportModel()->setDisplayOptions(options);
                    }
                });

            _addCheckCommand(
                "Blue",
                "Show the blue channel.",
                [appWeak](const nlohmann::json& args)
                {
                    const bool value = args.at("value").get<bool>();
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getViewportModel()->getDisplayOptions();
                        options.channels = value ?
                            ftk::ChannelDisplay::Blue :
                            ftk::ChannelDisplay::Color;
                        app->getViewportModel()->setDisplayOptions(options);
                    }
                });

            _addCheckCommand(
                "Alpha",
                "Show the alpha channel.",
                [appWeak](const nlohmann::json& args)
                {
                    const bool value = args.at("value").get<bool>();
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getViewportModel()->getDisplayOptions();
                        options.channels = value ?
                            ftk::ChannelDisplay::Alpha :
                            ftk::ChannelDisplay::Color;
                        app->getViewportModel()->setDisplayOptions(options);
                    }
                });

            _addCheckCommand(
                "Negative",
                "Show the image as a negative.",
                [appWeak](const nlohmann::json& args)
                {
                    const bool value = args.at("value").get<bool>();
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getViewportModel()->getDisplayOptions();
                        options.negative = value;
                        app->getViewportModel()->setDisplayOptions(options);
                    }
                });

            _addCheckCommand(
                "MirrorHorizontal",
                "Mirror the image horizontally.",
                [appWeak](const nlohmann::json& args)
                {
                    const bool value = args.at("value").get<bool>();
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getViewportModel()->getDisplayOptions();
                        options.mirror.x = value;
                        app->getViewportModel()->setDisplayOptions(options);
                    }
                });

            _addCheckCommand(
                "MirrorVertical",
                "Mirror the image vertically.",
                [appWeak](const nlohmann::json& args)
                {
                    const bool value = args.at("value").get<bool>();
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getViewportModel()->getDisplayOptions();
                        options.mirror.y = value;
                        app->getViewportModel()->setDisplayOptions(options);
                    }
                });

            _addCommand(
                "AspectRatio_0",
                "Set the aspect ratio to the default.",
                [appWeak](const nlohmann::json&)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getViewportModel()->getAspectRatioOptions();
                        options.index = 0;
                        app->getViewportModel()->setAspectRatioOptions(options);
                    }
                });

            _addCheckCommand(
                "Outline",
                "Toggle the outline.",
                [appWeak](const nlohmann::json& args)
                {
                    const bool value = args.at("value").get<bool>();
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getViewportModel()->getBackgroundOptions();
                        options.outline.enabled = value;
                        app->getViewportModel()->setBackgroundOptions(options);
                    }
                });

            _addCheckCommand(
                "Grid",
                "Toggle the grid.",
                [appWeak](const nlohmann::json& args)
                {
                    const bool value = args.at("value").get<bool>();
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getViewportModel()->getForegroundOptions();
                        options.grid.enabled = value;
                        app->getViewportModel()->setForegroundOptions(options);
                    }
                });

            _addCheckCommand(
                "CenterMarker",
                "Toggle the center marker.",
                [appWeak](const nlohmann::json& args)
                {
                    const bool value = args.at("value").get<bool>();
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getViewportModel()->getForegroundOptions();
                        options.centerMarker.enabled = value;
                        app->getViewportModel()->setForegroundOptions(options);
                    }
                });

            _addCheckCommand(
                "HUD",
                "Toggle the HUD / information display.",
                [appWeak](const nlohmann::json& args)
                {
                    const bool value = args.at("value").get<bool>();
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getViewportModel()->getHUDOptions();
                        options.enabled = value;
                        app->getViewportModel()->setHUDOptions(options);
                    }
                });

            const models::AspectRatioOptions aspectRatioOptions;
            for (size_t i = 1; i < aspectRatioOptions.options.size(); ++i)
            {
                _addCommand(
                    ftk::Format("AspectRatio_{0}").arg(i),
                    ftk::Format("Set the aspect ratio to {0}.").
                        arg(getLabel(aspectRatioOptions.options[i])),
                    [appWeak, i](const nlohmann::json&)
                    {
                        if (auto app = appWeak.lock())
                        {
                            auto options = app->getViewportModel()->getAspectRatioOptions();
                            options.index = i;
                            app->getViewportModel()->setAspectRatioOptions(options);
                        }
                    });
                _actions[ftk::Format("AspectRatio_{0}").arg(i)] = ftk::Action::create(
                    "",
                    _command(ftk::Format("AspectRatio_{0}").arg(i)));

                // Register the shortcut.
                _addShortcut(
                    ftk::Format("AspectRatio_{0}").arg(i),
                    ftk::Format("Custom aspect ratio {0}").arg(i));
            }

            // Create the actions.
            _actions["Frame"] = ftk::Action::create(
                "Frame",
                "ViewFrame",
                _checkCommand("Frame"));
            _actions["ZoomReset"] = ftk::Action::create(
                "Zoom Reset",
                "ViewZoomReset",
                _command("ZoomReset"));
            _actions["ZoomIn"] = ftk::Action::create(
                "Zoom In",
                "ViewZoomIn",
                _command("ZoomIn"));
            _actions["ZoomOut"] = ftk::Action::create(
                "Zoom Out",
                "ViewZoomOut",
                _command("ZoomOut"));
            _actions["Center"] = ftk::Action::create(
                "Center",
                _command("Center"));
            _actions["Red"] = ftk::Action::create(
                "Red Channel",
                _checkCommand("Red"));
            _actions["Green"] = ftk::Action::create(
                "Green Channel",
                _checkCommand("Green"));
            _actions["Blue"] = ftk::Action::create(
                "Blue Channel",
                _checkCommand("Blue"));
            _actions["Alpha"] = ftk::Action::create(
                "Alpha Channel",
                _checkCommand("Alpha"));
            _actions["Negative"] = ftk::Action::create(
                "Negative",
                _checkCommand("Negative"));
            _actions["MirrorHorizontal"] = ftk::Action::create(
                "Mirror Horizontal",
                _checkCommand("MirrorHorizontal"));
            _actions["MirrorVertical"] = ftk::Action::create(
                "Mirror Vertical",
                _checkCommand("MirrorVertical"));
            _actions["AspectRatio_0"] = ftk::Action::create(
                "Default",
                _command("AspectRatio_0"));
            _actions["Outline"] = ftk::Action::create(
                "Outline",
                _checkCommand("Outline"));
            _actions["Grid"] = ftk::Action::create(
                "Grid",
                _checkCommand("Grid"));
            _actions["CenterMarker"] = ftk::Action::create(
                "Center Marker",
                _checkCommand("CenterMarker"));
            _actions["HUD"] = ftk::Action::create(
                "HUD / Information Display",
                _checkCommand("HUD"));

            // Register the shortcuts.
            _addShortcut("Frame", "Frame", ftk::Key::Backspace);
            _addShortcut("ZoomReset", "Zoom reset", ftk::Key::_0);
            _addShortcut("ZoomIn", "Zoom in", ftk::Key::Equals);
            _addShortcut("ZoomOut", "Zoom out", ftk::Key::Minus);
            _addShortcut("Center", "Center", ftk::Key::Backslash);
            _addShortcut("Red", "Red channel", ftk::Key::R);
            _addShortcut("Green", "Green channel", ftk::Key::G);
            _addShortcut("Blue", "Blue channel", ftk::Key::B);
            _addShortcut("Alpha", "Alpha channel", ftk::Key::A);
            _addShortcut("Negative", "Negative", ftk::KeyShortcut(ftk::Key::I, static_cast<int>(ftk::KeyModifier::Control)));
            _addShortcut("MirrorHorizontal", "Mirror horizontal", ftk::Key::H);
            _addShortcut("MirrorVertical", "Mirror vertical", ftk::Key::V);
            _addShortcut("AspectRatio_0", "Default aspect ratio");
            _addShortcut("Grid", "Grid", ftk::KeyShortcut(ftk::Key::G, static_cast<int>(ftk::KeyModifier::Control)));
            _addShortcut("Outline", "Outline");
            _addShortcut("CenterMarker", "Center marker");
            _addShortcut("HUD", "HUD / Information Display", ftk::KeyShortcut(ftk::Key::H, static_cast<int>(ftk::KeyModifier::Control)));

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

                    _actions["Negative"]->setChecked(value.negative);

                    _actions["MirrorHorizontal"]->setChecked(value.mirror.x);
                    _actions["MirrorVertical"]->setChecked(value.mirror.y);
                });

            p.aspectRatioOptionsObserver = ftk::Observer<models::AspectRatioOptions>::create(
                app->getViewportModel()->observeAspectRatioOptions(),
                [this](const models::AspectRatioOptions& value)
                {
                    _actions["AspectRatio_0"]->setChecked(0 == value.index);
                    for (size_t i = 1; i < value.options.size(); ++i)
                    {
                        auto& action = _actions[ftk::Format("AspectRatio_{0}").arg(i)];
                        action->setText(getLabel(value.options[i]));
                        action->setChecked(static_cast<int>(i) == value.index);
                    }
                });

            p.bgOptionsObserver = ftk::Observer<tl::BackgroundOptions>::create(
                app->getViewportModel()->observeBackgroundOptions(),
                [this](const tl::BackgroundOptions& value)
                {
                    _actions["Outline"]->setChecked(value.outline.enabled);
                });

            p.fgOptionsObserver = ftk::Observer<tl::ForegroundOptions>::create(
                app->getViewportModel()->observeForegroundOptions(),
                [this](const tl::ForegroundOptions& value)
                {
                    _actions["Grid"]->setChecked(value.grid.enabled);
                    _actions["CenterMarker"]->setChecked(value.centerMarker.enabled);
                });

            p.hudOptionsObserver = ftk::Observer<models::HUDOptions>::create(
                app->getViewportModel()->observeHUDOptions(),
                [this](const models::HUDOptions& value)
                {
                    _actions["HUD"]->setChecked(value.enabled);
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
