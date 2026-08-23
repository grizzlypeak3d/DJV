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
        class ColorActions;

        //! Color menu.
        class DJV_APP_API_TYPE ColorMenu : public ftk::Menu
        {
            FTK_NON_COPYABLE(ColorMenu);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<ColorActions>&,
                const std::shared_ptr<IWidget>& parent);

            ColorMenu() = default;

        public:
            DJV_APP_API ~ColorMenu();

            DJV_APP_API static std::shared_ptr<ColorMenu> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<ColorActions>&,
                const std::shared_ptr<IWidget>& parent = nullptr);
        };
    }
}
