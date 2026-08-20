// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/App/IActions.h>
#include <djv/Models/Export.h>

namespace djv
{
    namespace app
    {
        //! Playback actions.
        class DJV_API_TYPE PlaybackActions : public IActions
        {
            FTK_NON_COPYABLE(PlaybackActions);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&);

            PlaybackActions();

        public:
            DJV_API ~PlaybackActions();

            DJV_API static std::shared_ptr<PlaybackActions> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&);

        private:
            void _setPlayer(const std::shared_ptr<tl::Player>&);

            void _playbackUpdate(tl::Playback);
            void _loopUpdate(tl::Loop);

            FTK_PRIVATE();
        };
    }
}
