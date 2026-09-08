// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/ToolsMenu.h>

#include <djv/App/ToolsActions.h>
#include <djv/Models/ToolsModel.h>
#include <djv/App/App.h>

namespace djv
{
    namespace app
    {
        void ToolsMenu::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<ToolsActions>& toolsActions,
            const std::shared_ptr<IWidget>& parent)
        {
            Menu::_init(context, parent);

            auto actions = toolsActions->getActions();
            std::map<std::string, std::vector<std::string> > tools;
            for (const auto& tool : app->getToolsModel()->getTools())
            {
                tools[tool.sort].push_back(tool.name);
            }
            for (const auto& i : tools)
            {
                for (const auto& j : i.second)
                {
                    addAction(actions[j]);
                }
            }
        }

        ToolsMenu::~ToolsMenu()
        {}

        std::shared_ptr<ToolsMenu> ToolsMenu::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<ToolsActions>& toolsActions,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<ToolsMenu>(new ToolsMenu);
            out->_init(context, app, toolsActions, parent);
            return out;
        }
    }
}
