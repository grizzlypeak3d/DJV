// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <ftk/UI/Menu.h>

namespace djv
{
    namespace app
    {
        class TimelineActions;

        //! Timeline menu.
        class TimelineMenu : public ftk::Menu
        {
            FTK_NON_COPYABLE(TimelineMenu);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<TimelineActions>&,
                const std::shared_ptr<IWidget>& parent);

            TimelineMenu();

        public:
            ~TimelineMenu();

            static std::shared_ptr<TimelineMenu> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<TimelineActions>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

        private:
            FTK_PRIVATE();
        };
    }
}
