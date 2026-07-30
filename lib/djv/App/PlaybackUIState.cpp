// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/PlaybackUIState.h>

#include <algorithm>
#include <cmath>

namespace djv
{
    namespace app
    {
        namespace
        {
            const auto doubleClickDuration = std::chrono::milliseconds(500);
            const auto overlayHideDelay = std::chrono::milliseconds(2200);
            const auto overlayAnimationDuration = std::chrono::milliseconds(180);
        }

        bool PlaybackDoubleClickDetector::release(
            int x,
            int y,
            const Clock::time_point& time,
            int maxDistance)
        {
            bool out = false;
            if (_hasClick)
            {
                const auto elapsed = time - _time;
                const int dx = x - _x;
                const int dy = y - _y;
                const int maxDistance2 = maxDistance * maxDistance;
                out =
                    elapsed >= Clock::duration::zero() &&
                    elapsed <= doubleClickDuration &&
                    dx * dx + dy * dy <= maxDistance2;
            }

            if (out)
            {
                reset();
            }
            else
            {
                _hasClick = true;
                _x = x;
                _y = y;
                _time = time;
            }
            return out;
        }

        void PlaybackDoubleClickDetector::reset()
        {
            _hasClick = false;
            _x = 0;
            _y = 0;
            _time = Clock::time_point();
        }

        void PlaybackOverlayState::setFullScreen(
            bool value,
            const Clock::time_point& time)
        {
            _fullScreen = value;
            _pinned = false;
            _lastActivity = time;
            _lastTick = time;
            _visibility = value ? 1.0 : 0.0;
        }

        bool PlaybackOverlayState::isFullScreen() const
        {
            return _fullScreen;
        }

        void PlaybackOverlayState::setPinned(
            bool value,
            const Clock::time_point& time)
        {
            _pinned = _fullScreen && value;
            activity(time);
        }

        bool PlaybackOverlayState::isPinned() const
        {
            return _pinned;
        }

        void PlaybackOverlayState::activity(const Clock::time_point& time)
        {
            if (_fullScreen)
            {
                _lastActivity = time;
            }
        }

        bool PlaybackOverlayState::wantsVisible(
            const Clock::time_point& time) const
        {
            return
                _fullScreen &&
                (_pinned || time - _lastActivity < overlayHideDelay);
        }

        bool PlaybackOverlayState::tick(const Clock::time_point& time)
        {
            if (!_fullScreen)
            {
                return false;
            }

            const double previous = _visibility;
            const auto elapsed = std::max(
                Clock::duration::zero(),
                time - _lastTick);
            const double duration = static_cast<double>(
                std::chrono::duration_cast<std::chrono::microseconds>(
                    overlayAnimationDuration).count());
            const double step =
                duration > 0.0 ?
                static_cast<double>(
                    std::chrono::duration_cast<std::chrono::microseconds>(
                        elapsed).count()) / duration :
                1.0;

            if (wantsVisible(time))
            {
                _visibility = std::min(1.0, _visibility + step);
            }
            else
            {
                _visibility = std::max(0.0, _visibility - step);
            }
            _lastTick = time;
            return std::abs(_visibility - previous) > .0001;
        }

        double PlaybackOverlayState::getVisibility() const
        {
            return _visibility;
        }

        double getPlaybackProgress(
            double currentTime,
            double startTime,
            double duration)
        {
            if (!std::isfinite(currentTime) ||
                !std::isfinite(startTime) ||
                !std::isfinite(duration) ||
                duration <= 0.0)
            {
                return 0.0;
            }
            return std::clamp(
                (currentTime - startTime) / duration,
                0.0,
                1.0);
        }
    }
}
