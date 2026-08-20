// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/Models/Export.h>

#include <ftk/UI/Menu.h>

namespace djv
{
    namespace app
    {
        class AudioActions;

        //! Audio menu.
        class DJV_API_TYPE AudioMenu : public ftk::Menu
        {
            FTK_NON_COPYABLE(AudioMenu);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<AudioActions>&,
                const std::shared_ptr<IWidget>& parent);

            AudioMenu() = default;

        public:
            DJV_API ~AudioMenu();

            DJV_API static std::shared_ptr<AudioMenu> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<AudioActions>&,
                const std::shared_ptr<IWidget>& parent = nullptr);
        };
    }
}
