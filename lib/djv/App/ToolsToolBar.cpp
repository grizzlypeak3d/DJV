// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/ToolsToolBar.h>

#include <djv/Models/ToolsModel.h>

namespace djv
{
    namespace app
    {
        void ToolsToolBar::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::map<std::string, std::shared_ptr<ftk::Action> >& actions,
            const std::shared_ptr<IWidget>& parent)
        {
            ToolBar::_init(context, ftk::Orientation::Horizontal, parent);

            setMarginRole(ftk::SizeRole::MarginInside);

            auto tools = models::getToolsInToolbar();
            auto tmp = actions;
            for (const auto tool : tools)
            {
                addAction(tmp[getLabel(tool)]);
            }
        }

        ToolsToolBar::~ToolsToolBar()
        {}

        std::shared_ptr<ToolsToolBar> ToolsToolBar::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::map<std::string, std::shared_ptr<ftk::Action> >& actions,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<ToolsToolBar>(new ToolsToolBar);
            out->_init(context, actions, parent);
            return out;
        }
    }
}
