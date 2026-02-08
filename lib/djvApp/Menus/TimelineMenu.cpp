// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djvApp/Menus/TimelineMenu.h>

#include <djvApp/Actions/TimelineActions.h>
#include <djvApp/App.h>
#include <djvApp/MainWindow.h>

#include <tlRender/UI/TimelineWidget.h>

namespace djv
{
    namespace app
    {
        struct TimelineMenu::Private
        {};

        void TimelineMenu::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<TimelineActions>& timelineActions,
            const std::shared_ptr<IWidget>& parent)
        {
            Menu::_init(context, parent);
            FTK_P();

            auto actions = timelineActions->getActions();
            addAction(actions["Minimize"]);
            addAction(actions["FrameView"]);
            addAction(actions["ScrollBars"]);
            addAction(actions["AutoScroll"]);
            addAction(actions["StopOnScrub"]);
            addDivider();
            addAction(actions["Thumbnails"]);
            auto thumbnailSizeMenu = addSubMenu("Thumbnail Size");
            thumbnailSizeMenu->addAction(actions["ThumbnailSizeSmall"]);
            thumbnailSizeMenu->addAction(actions["ThumbnailSizeMedium"]);
            thumbnailSizeMenu->addAction(actions["ThumbnailSizeLarge"]);
        }

        TimelineMenu::TimelineMenu() :
            _p(new Private)
        {}

        TimelineMenu::~TimelineMenu()
        {}

        std::shared_ptr<TimelineMenu> TimelineMenu::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<TimelineActions>& timelineActions,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<TimelineMenu>(new TimelineMenu);
            out->_init(context, timelineActions, parent);
            return out;
        }
    }
}
