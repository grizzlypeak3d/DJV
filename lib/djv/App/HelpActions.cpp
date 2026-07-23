// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/HelpActions.h>

#include <djv/App/App.h>
#include <djv/App/MainWindow.h>

#include <ftk/Core/OS.h>

namespace djv
{
    namespace app
    {
        struct HelpActions::Private
        {
        };

        void HelpActions::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<MainWindow>& mainWindow)
        {
            IActions::_init(context, app, "Help");
            FTK_P();

            std::weak_ptr<MainWindow> mainWindowWeak(mainWindow);

            // Register the commands.
            _addCommand(
                "Documentation",
                "Open the documentation in a web browser.",
                [](const nlohmann::json&)
                {
                    try
                    {
                        ftk::openURL("https://grizzlypeak3d.github.io/DJV/index.html");
                    }
                    catch (const std::exception&)
                    {}
                });

            _addCommand(
                "About",
                "Show the about dialog.",
                [mainWindowWeak](const nlohmann::json&)
                {
                    if (auto mainWindow = mainWindowWeak.lock())
                    {
                        mainWindow->showAboutDialog();
                    }
                });

            _addCommand(
                "SysInfo",
                "Show the system information dialog.",
                [mainWindowWeak](const nlohmann::json&)
                {
                    if (auto mainWindow = mainWindowWeak.lock())
                    {
                        mainWindow->showSysInfoDialog();
                    }
                });

            // Create the actions.
            _actions["Documentation"] = ftk::Action::create(
                "Documentation",
                _command("Documentation"));
            _actions["About"] = ftk::Action::create(
                "About",
                _command("About"));
            _actions["SysInfo"] = ftk::Action::create(
                "System Information",
                _command("SysInfo"));
        }

        HelpActions::HelpActions() :
            _p(new Private)
        {}

        HelpActions::~HelpActions()
        {}

        std::shared_ptr<HelpActions> HelpActions::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<MainWindow>& mainWindow)
        {
            auto out = std::shared_ptr<HelpActions>(new HelpActions);
            out->_init(context, app, mainWindow);
            return out;
        }
    }
}
