// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/Models/Export.h>

#include <ftk/UI/Menu.h>

namespace djv
{
    namespace app
    {
        class HelpActions;

        //! Help menu.
        class DJV_API_TYPE HelpMenu : public ftk::Menu
        {
            FTK_NON_COPYABLE(HelpMenu);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<HelpActions>&,
                const std::shared_ptr<IWidget>& parent);

            HelpMenu() = default;

        public:
            DJV_API ~HelpMenu();

            DJV_API static std::shared_ptr<HelpMenu> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<HelpActions>&,
                const std::shared_ptr<IWidget>& parent = nullptr);
        };
    }
}
