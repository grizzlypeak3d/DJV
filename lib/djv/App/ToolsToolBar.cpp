// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/ToolsToolBar.h>

#include <djv/App/App.h>
#include <djv/Models/ToolsModel.h>

namespace djv
{
    namespace app
    {
        void ToolsToolBar::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::map<std::string, std::shared_ptr<ftk::Action> >& actions,
            const std::shared_ptr<IWidget>& parent)
        {
            ToolBar::_init(context, ftk::Orientation::Horizontal, parent);

            setMarginRole(ftk::SizeRole::MarginInside);

            std::map<std::string, std::vector<models::ToolInfo> > tools;
            for (const auto& tool : app->getToolsModel()->getTools())
            {
                tools[tool.sort].push_back(tool);
            }
            
            auto tmp = actions;
            for (const auto& i : tools)
            {
                for (const auto& j : i.second)
                {
                    if (j.toolBar)
                    {
                        addAction(tmp[j.name]);
                    }
                }
            }
        }

        ToolsToolBar::~ToolsToolBar()
        {}

        std::shared_ptr<ToolsToolBar> ToolsToolBar::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::map<std::string, std::shared_ptr<ftk::Action> >& actions,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<ToolsToolBar>(new ToolsToolBar);
            out->_init(context, app, actions, parent);
            return out;
        }
    }
}
