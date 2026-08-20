// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/Models/Export.h>

#include <tlRender/Timeline/Player.h>

#include <ftk/UI/Menu.h>

namespace djv
{
    namespace app
    {
        class PlaybackActions;

        //! Playback menu.
        class DJV_API_TYPE PlaybackMenu : public ftk::Menu
        {
            FTK_NON_COPYABLE(PlaybackMenu);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<PlaybackActions>&,
                const std::shared_ptr<IWidget>& parent);

            PlaybackMenu() = default;

        public:
            DJV_API ~PlaybackMenu();

            DJV_API static std::shared_ptr<PlaybackMenu> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<PlaybackActions>&,
                const std::shared_ptr<IWidget>& parent = nullptr);
        };
    }
}
