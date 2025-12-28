// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djvApp/Actions/IActions.h>

namespace djv
{
    namespace app
    {
        //! Playback actions.
        class PlaybackActions : public IActions
        {
            FTK_NON_COPYABLE(PlaybackActions);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&);

            PlaybackActions();

        public:
            ~PlaybackActions();

            static std::shared_ptr<PlaybackActions> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&);

        private:
            void _setPlayer(const std::shared_ptr<tl::timeline::Player>&);

            void _playbackUpdate(tl::timeline::Playback);
            void _loopUpdate(tl::timeline::Loop);

            FTK_PRIVATE();
        };
    }
}
