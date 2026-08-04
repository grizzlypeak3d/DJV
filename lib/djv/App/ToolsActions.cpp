// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/ToolsActions.h>

#include <djv/App/App.h>
#include <djv/Models/SettingsModel.h>
#include <djv/Models/ToolsModel.h>

#include <algorithm>

#include <ftk/Core/Format.h>

namespace djv
{
    namespace app
    {
        struct ToolsActions::Private
        {
            std::shared_ptr<ftk::ListObserver<std::string> > openObserver;
        };

        void ToolsActions::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app)
        {
            IActions::_init(context, app, "Tools");
            FTK_P();

            auto appWeak = std::weak_ptr<App>(app);
            for (const auto& tool : app->getToolsModel()->getTools())
            {
                // Register the command.
                _addCheckCommand(
                    tool.name,
                    ftk::Format("Toggle the {0} tool.").arg(tool.name),
                    [appWeak, tool](const nlohmann::json& args)
                    {
                        const bool value = args.at("value").get<bool>();
                        if (auto app = appWeak.lock())
                        {
                            app->getToolsModel()->setToolOpen(tool.name, value);
                            if (value)
                            {
                                // Opening a tool while the panel is hidden
                                // would otherwise do nothing that can be
                                // seen, which reads as the button being
                                // broken.
                                auto window =
                                    app->getSettingsModel()->getWindow();
                                if (!window.tools)
                                {
                                    window.tools = true;
                                    app->getSettingsModel()->setWindow(window);
                                }
                            }
                        }
                    });

                // Create the action.
                _actions[tool.name] = ftk::Action::create(
                    tool.name,
                    tool.icon,
                    _checkCommand(tool.name));

                // Register the shortcut.
                _addShortcut(tool.name, tool.name, tool.shortcut);
            }

            _shortcutsUpdate(app->getSettingsModel()->getShortcuts());

            p.openObserver = ftk::ListObserver<std::string>::create(
                app->getToolsModel()->observeOpenTools(),
                [this](const std::vector<std::string>& value)
                {
                    for (auto i : _actions)
                    {
                        i.second->setChecked(
                            std::find(value.begin(), value.end(), i.first) !=
                            value.end());
                    }
                });
        }

        ToolsActions::ToolsActions() :
            _p(new Private)
        {}

        ToolsActions::~ToolsActions()
        {}

        std::shared_ptr<ToolsActions> ToolsActions::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app)
        {
            auto out = std::shared_ptr<ToolsActions>(new ToolsActions);
            out->_init(context, app);
            return out;
        }
    }
}
