// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djvApp/Widgets/WindowToolBar.h>

namespace djv
{
    namespace app
    {
        void WindowToolBar::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::map<std::string, std::shared_ptr<ftk::Action> >& actions,
            const std::shared_ptr<IWidget>& parent)
        {
            ToolBar::_init(context, ftk::Orientation::Horizontal, parent);

            setMarginRole(ftk::SizeRole::MarginInside);

            auto tmp = actions;
            addAction(tmp["FullScreen"]);
            addAction(tmp["Secondary"]);
        }

        WindowToolBar::~WindowToolBar()
        {}

        std::shared_ptr<WindowToolBar> WindowToolBar::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::map<std::string, std::shared_ptr<ftk::Action> >& actions,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<WindowToolBar>(new WindowToolBar);
            out->_init(context, actions, parent);
            return out;
        }
    }
}
