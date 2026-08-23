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
        class MainWindow;
        class WindowActions;

        //! Window menu.
        class DJV_APP_API_TYPE WindowMenu : public ftk::Menu
        {
            FTK_NON_COPYABLE(WindowMenu);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<MainWindow>&,
                const std::shared_ptr<WindowActions>&,
                const std::shared_ptr<IWidget>& parent);

            WindowMenu();

        public:
            DJV_APP_API ~WindowMenu();

            DJV_APP_API static std::shared_ptr<WindowMenu> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<MainWindow>&,
                const std::shared_ptr<WindowActions>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

        private:
            FTK_PRIVATE();
        };
    }
}
