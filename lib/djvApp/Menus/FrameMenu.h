// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <tlRender/Timeline/Player.h>

#include <ftk/UI/Menu.h>

namespace djv
{
    namespace app
    {
        class FrameActions;

        //! Frame menu.
        class FrameMenu : public ftk::Menu
        {
            FTK_NON_COPYABLE(FrameMenu);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<FrameActions>&,
                const std::shared_ptr<IWidget>& parent);

            FrameMenu() = default;

        public:
            ~FrameMenu();

            static std::shared_ptr<FrameMenu> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<FrameActions>&,
                const std::shared_ptr<IWidget>& parent = nullptr);
        };
    }
}

