// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/WindowActions.h>

#include <djv/App/App.h>
#include <djv/App/MainWindow.h>

namespace djv
{
    namespace app
    {
        struct WindowActions::Private
        {
            std::shared_ptr<ftk::Observer<bool> > fullScreenObserver;
            std::shared_ptr<ftk::Observer<bool> > floatOnTopObserver;
            std::shared_ptr<ftk::Observer<bool> > secondaryObserver;
            std::shared_ptr<ftk::Observer<models::WindowSettings> > settingsObserver;
        };

        void WindowActions::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<MainWindow>& mainWindow)
        {
            IActions::_init(context, app, "Window");
            FTK_P();

            auto appWeak = std::weak_ptr<App>(app);
            _actions["FullScreen"] = ftk::Action::create(
                "Full Screen",
                "WindowFullScreen",
                [appWeak](bool value)
                {
                    if (auto app = appWeak.lock())
                    {
                        app->getMainWindow()->setFullScreen(value);
                    }
                });

            _actions["FloatOnTop"] = ftk::Action::create(
                "Float On Top",
                [appWeak](bool value)
                {
                    if (auto app = appWeak.lock())
                    {
                        app->getMainWindow()->setFloatOnTop(value);
                    }
                });

            _actions["Secondary"] = ftk::Action::create(
                "Secondary",
                "WindowSecondary",
                [appWeak](bool value)
                {
                    if (auto app = appWeak.lock())
                    {
                        app->setSecondaryWindow(value);
                    }
                });

            _actions["FileToolBar"] = ftk::Action::create(
                "File Tool Bar",
                [appWeak](bool value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getSettingsModel()->getWindow();
                        options.fileToolBar = value;
                        app->getSettingsModel()->setWindow(options);
                    }
                });

            _actions["CompareToolBar"] = ftk::Action::create(
                "Compare Tool Bar",
                [appWeak](bool value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getSettingsModel()->getWindow();
                        options.compareToolBar = value;
                        app->getSettingsModel()->setWindow(options);
                    }
                });

            _actions["WindowToolBar"] = ftk::Action::create(
                "Window Tool Bar",
                [appWeak](bool value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getSettingsModel()->getWindow();
                        options.windowToolBar = value;
                        app->getSettingsModel()->setWindow(options);
                    }
                });

            _actions["ViewToolBar"] = ftk::Action::create(
                "View Tool Bar",
                [appWeak](bool value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getSettingsModel()->getWindow();
                        options.viewToolBar = value;
                        app->getSettingsModel()->setWindow(options);
                    }
                });

            _actions["ToolsToolBar"] = ftk::Action::create(
                "Tools Tool Bar",
                [appWeak](bool value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getSettingsModel()->getWindow();
                        options.toolsToolBar = value;
                        app->getSettingsModel()->setWindow(options);
                    }
                });

            _actions["TabBar"] = ftk::Action::create(
                "Tab Bar",
                [appWeak](bool value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getSettingsModel()->getWindow();
                        options.tabBar = value;
                        app->getSettingsModel()->setWindow(options);
                    }
                });

            _actions["Timeline"] = ftk::Action::create(
                "Timeline",
                [appWeak](bool value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getSettingsModel()->getWindow();
                        options.timeline = value;
                        app->getSettingsModel()->setWindow(options);
                    }
                });

            _actions["BottomToolBar"] = ftk::Action::create(
                "Bottom Tool Bar",
                [appWeak](bool value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getSettingsModel()->getWindow();
                        options.bottomToolBar = value;
                        app->getSettingsModel()->setWindow(options);
                    }
                });

            _actions["StatusToolBar"] = ftk::Action::create(
                "Status Tool Bar",
                [appWeak](bool value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getSettingsModel()->getWindow();
                        options.statusToolBar = value;
                        app->getSettingsModel()->setWindow(options);
                    }
                });

            _tooltips =
            {
                { "FullScreen", "Toggle the window full screen." },
                { "Secondary", "Toggle the secondary window." }
            };

            _shortcutsUpdate(app->getSettingsModel()->getShortcuts());

            p.fullScreenObserver = ftk::Observer<bool>::create(
                mainWindow->observeFullScreen(),
                [this](bool value)
                {
                    _actions["FullScreen"]->setChecked(value);
                });

            p.floatOnTopObserver = ftk::Observer<bool>::create(
                mainWindow->observeFloatOnTop(),
                [this](bool value)
                {
                    _actions["FloatOnTop"]->setChecked(value);
                });

            p.secondaryObserver = ftk::Observer<bool>::create(
                app->observeSecondaryWindow(),
                [this](bool value)
                {
                    _actions["Secondary"]->setChecked(value);
                });

            p.settingsObserver = ftk::Observer<models::WindowSettings>::create(
                app->getSettingsModel()->observeWindow(),
                [this](const models::WindowSettings& value)
                {
                    _actions["FileToolBar"]->setChecked(value.fileToolBar);
                    _actions["CompareToolBar"]->setChecked(value.compareToolBar);
                    _actions["WindowToolBar"]->setChecked(value.windowToolBar);
                    _actions["ViewToolBar"]->setChecked(value.viewToolBar);
                    _actions["ToolsToolBar"]->setChecked(value.toolsToolBar);
                    _actions["TabBar"]->setChecked(value.tabBar);
                    _actions["Timeline"]->setChecked(value.timeline);
                    _actions["BottomToolBar"]->setChecked(value.bottomToolBar);
                    _actions["StatusToolBar"]->setChecked(value.statusToolBar);
                });
        }

        WindowActions::WindowActions() :
            _p(new Private)
        {}

        WindowActions::~WindowActions()
        {}

        std::shared_ptr<WindowActions> WindowActions::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<MainWindow>& mainWindow)
        {
            auto out = std::shared_ptr<WindowActions>(new WindowActions);
            out->_init(context, app, mainWindow);
            return out;
        }
    }
}
