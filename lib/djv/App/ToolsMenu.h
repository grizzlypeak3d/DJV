// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <ftk/UI/Menu.h>

namespace djv
{
    namespace app
    {
        class ToolsActions;

        //! Tools menu.
        class ToolsMenu : public ftk::Menu
        {
            FTK_NON_COPYABLE(ToolsMenu);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<ToolsActions>&,
                const std::shared_ptr<IWidget>& parent);

            ToolsMenu() = default;

        public:
            ~ToolsMenu();

            static std::shared_ptr<ToolsMenu> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<ToolsActions>&,
                const std::shared_ptr<IWidget>& parent = nullptr);
        };
    }
}
