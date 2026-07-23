// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/ToolsActions.h>

#include <djv/App/App.h>
#include <djv/Models/ToolsModel.h>

#include <ftk/Core/Format.h>

namespace djv
{
    namespace app
    {
        struct ToolsActions::Private
        {
            std::shared_ptr<ftk::Observer<std::string> > activeObserver;
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
                            auto toolsModel = app->getToolsModel();
                            if (value)
                            {
                                toolsModel->setActiveTool(tool.name);
                            }
                            else if (toolsModel->getActiveTool() == tool.name)
                            {
                                toolsModel->setActiveTool(std::string());
                            }
                        }
                    });

                // Create the action.
                _actions[tool.name] = ftk::Action::create(
                    tool.name,
                    tool.icon,
                    _checkCommand(tool.name));
            }

            _shortcutsUpdate(app->getSettingsModel()->getShortcuts());

            p.activeObserver = ftk::Observer<std::string>::create(
                app->getToolsModel()->observeActiveTool(),
                [this](const std::string& value)
                {
                    for (auto i : _actions)
                    {
                        i.second->setChecked(i.first == value);
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
