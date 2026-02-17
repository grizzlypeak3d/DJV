// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/ViewMenu.h>

#include <djv/App/ViewActions.h>

namespace djv
{
    namespace app
    {
        void ViewMenu::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<ViewActions>& viewActions,
            const std::shared_ptr<IWidget>& parent)
        {
            Menu::_init(context, parent);

            auto actions = viewActions->getActions();
            addAction(actions["Frame"]);
            addAction(actions["ZoomReset"]);
            addAction(actions["ZoomIn"]);
            addAction(actions["ZoomOut"]);
            addDivider();
            addAction(actions["Red"]);
            addAction(actions["Green"]);
            addAction(actions["Blue"]);
            addAction(actions["Alpha"]);
            addDivider();
            addAction(actions["MirrorHorizontal"]);
            addAction(actions["MirrorVertical"]);
            addDivider();
            addAction(actions["Grid"]);
            addAction(actions["HUD"]);
        }

        ViewMenu::~ViewMenu()
        {}

        std::shared_ptr<ViewMenu> ViewMenu::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<ViewActions>& viewActions,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<ViewMenu>(new ViewMenu);
            out->_init(context, viewActions, parent);
            return out;
        }
    }
}
