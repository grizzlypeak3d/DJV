// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/App/Export.h>
#include <djv/Models/Export.h>

#include <ftk/UI/Menu.h>

namespace djv
{
    namespace app
    {
        class App;
        class ToolsActions;

        //! Tools menu.
        class DJV_APP_API_TYPE ToolsMenu : public ftk::Menu
        {
            FTK_NON_COPYABLE(ToolsMenu);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<ToolsActions>&,
                const std::shared_ptr<IWidget>& parent);

            ToolsMenu() = default;

        public:
            DJV_APP_API ~ToolsMenu();

            DJV_APP_API static std::shared_ptr<ToolsMenu> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<ToolsActions>&,
                const std::shared_ptr<IWidget>& parent = nullptr);
        };
    }
}
