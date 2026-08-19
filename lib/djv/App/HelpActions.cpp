// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/HelpActions.h>

#include <djv/App/App.h>
#include <djv/App/MainWindow.h>

#include <djv/Models/AppInfoModel.h>

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

            // Installed beside the application rather than on the web, so
            // that what it describes is the version that is running.
            const std::string docsURL = app->getAppInfoModel()->getDocsURL();

            // Register the commands.
            _addCommand(
                "Documentation",
                "Open the documentation in a web browser.",
                [docsURL](const nlohmann::json&)
                {
                    if (docsURL.empty())
                        return;
                    try
                    {
                        ftk::openURL(docsURL);
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
            if (docsURL.empty())
            {
                // A build that was not installed has none. Saying so is
                // better than a menu item that does nothing when it is
                // clicked.
                _actions["Documentation"]->setEnabled(false);
                _actions["Documentation"]->setTooltip(
                    "The documentation is installed with the application,\n"
                    "and this build was not installed.");
            }
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
