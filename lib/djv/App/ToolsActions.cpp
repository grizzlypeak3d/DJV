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
                auto action = ftk::Action::create(
                    tool.name,
                    tool.icon,
                    [appWeak, tool](bool value)
                    {
                        if (auto app = appWeak.lock())
                        {
                            auto toolsModel = app->getToolsModel();
                            const auto active = toolsModel->getActiveTool();
                            toolsModel->setActiveTool(tool.name != active ? tool.name : std::string());
                        }
                    });
                _actions[tool.name] = action;
                _tooltips[tool.name] = ftk::Format("Toggle the {0} tool.").arg(tool.name);
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
