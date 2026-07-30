// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <chrono>

namespace djv
{
    namespace app
    {
        //! Detect consecutive releases that form a pointer double-click.
        class PlaybackDoubleClickDetector
        {
        public:
            using Clock = std::chrono::steady_clock;

            bool release(
                int x,
                int y,
                const Clock::time_point&,
                int maxDistance = 6);

            void reset();

        private:
            bool _hasClick = false;
            int _x = 0;
            int _y = 0;
            Clock::time_point _time;
        };

        //! Full-screen playback controls visibility and animation state.
        class PlaybackOverlayState
        {
        public:
            using Clock = std::chrono::steady_clock;

            void setFullScreen(bool, const Clock::time_point&);
            bool isFullScreen() const;

            void setPinned(bool, const Clock::time_point&);
            bool isPinned() const;

            void activity(const Clock::time_point&);
            bool wantsVisible(const Clock::time_point&) const;

            //! Advance the slide animation and return whether it changed.
            bool tick(const Clock::time_point&);

            //! Current slide visibility in the inclusive range [0, 1].
            double getVisibility() const;

        private:
            bool _fullScreen = false;
            bool _pinned = false;
            double _visibility = 0.0;
            Clock::time_point _lastActivity;
            Clock::time_point _lastTick;
        };

        //! Normalize absolute media progress to the inclusive range [0, 1].
        double getPlaybackProgress(
            double currentTime,
            double startTime,
            double duration);
    }
}
