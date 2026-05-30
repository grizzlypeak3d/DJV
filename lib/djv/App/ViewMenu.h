// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/Models/ViewportModel.h>

#include <ftk/UI/Menu.h>

namespace djv
{
    namespace app
    {
        class App;
        class ViewActions;

        //! View menu.
        class ViewMenu : public ftk::Menu
        {
            FTK_NON_COPYABLE(ViewMenu);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<ViewActions>&,
                const std::shared_ptr<IWidget>& parent);

            ViewMenu();

        public:
            ~ViewMenu();

            static std::shared_ptr<ViewMenu> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<ViewActions>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

        private:
            FTK_PRIVATE();
        };
    }
}
