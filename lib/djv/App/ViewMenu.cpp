// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/ViewMenu.h>

#include <djv/App/App.h>
#include <djv/App/ViewActions.h>

#include <ftk/Core/Format.h>

namespace djv
{
    namespace app
    {
        struct ViewMenu::Private
        {
            std::shared_ptr<ftk::Menu> aspectRatioMenu;
        };

        void ViewMenu::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<ViewActions>& viewActions,
            const std::shared_ptr<IWidget>& parent)
        {
            Menu::_init(context, parent);
            FTK_P();

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
            p.aspectRatioMenu = addSubMenu("Aspect Ratio");
            const models::AspectRatioOptions aspectRatioOptions;
            for (size_t i = 0; i < aspectRatioOptions.options.size(); ++i)
            {
                p.aspectRatioMenu->addAction(actions[ftk::Format("AspectRatio_{0}").arg(i)]);
            }
            addDivider();
            addAction(actions["Grid"]);
            addAction(actions["Outline"]);
            addAction(actions["CenterMarker"]);
            addAction(actions["HUD"]);
        }

        ViewMenu::ViewMenu() :
            _p(new Private)
        {}

        ViewMenu::~ViewMenu()
        {}

        std::shared_ptr<ViewMenu> ViewMenu::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<ViewActions>& viewActions,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<ViewMenu>(new ViewMenu);
            out->_init(context, app, viewActions, parent);
            return out;
        }
    }
}
