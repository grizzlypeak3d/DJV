// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djvApp/Actions/HelpActions.h>

#include <djvApp/App.h>
#include <djvApp/MainWindow.h>

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

            _actions["Documentation"] = ftk::Action::create(
                "Documentation",
                []
                {
                    try
                    {
                        ftk::openURL("https://grizzlypeak3d.github.io/DJV/index.html");
                    }
                    catch (const std::exception&)
                    {}
                });

            std::weak_ptr<MainWindow> mainWindowWeak(mainWindow);
            _actions["About"] = ftk::Action::create(
                "About",
                [mainWindowWeak]
                {
                    if (auto mainWindow = mainWindowWeak.lock())
                    {
                        mainWindow->showAboutDialog();
                    }
                });
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
