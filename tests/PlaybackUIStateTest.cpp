// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/PlaybackUIState.h>

#include <cmath>
#include <iostream>

namespace
{
    bool check(bool value, const char* message)
    {
        if (!value)
        {
            std::cerr << message << std::endl;
        }
        return value;
    }
}

int main()
{
    using namespace djv::app;
    using Clock = PlaybackOverlayState::Clock;
    using namespace std::chrono_literals;

    bool ok = true;
    const auto t0 = Clock::time_point();

    PlaybackDoubleClickDetector doubleClick;
    ok &= check(
        !doubleClick.release(10, 10, t0),
        "The first click must not toggle full-screen.");
    ok &= check(
        doubleClick.release(14, 12, t0 + 200ms),
        "A nearby second click must toggle full-screen.");
    ok &= check(
        !doubleClick.release(10, 10, t0 + 1s),
        "A new click sequence must start after a double-click.");
    ok &= check(
        !doubleClick.release(30, 30, t0 + 1100ms),
        "A distant second click must not toggle full-screen.");

    PlaybackOverlayState overlay;
    overlay.setFullScreen(true, t0);
    ok &= check(
        overlay.isFullScreen() && overlay.wantsVisible(t0),
        "The overlay must be visible when full-screen starts.");
    overlay.tick(t0 + 2190ms);
    overlay.tick(t0 + 2210ms);
    ok &= check(
        overlay.getVisibility() > 0.0 &&
        overlay.getVisibility() < 1.0,
        "The overlay must slide out after pointer inactivity.");
    overlay.tick(t0 + 2390ms);
    ok &= check(
        std::abs(overlay.getVisibility()) < .0001,
        "The overlay must finish outside the window.");
    overlay.activity(t0 + 2400ms);
    overlay.tick(t0 + 2416ms);
    ok &= check(
        overlay.getVisibility() > 0.0,
        "Pointer activity must slide the overlay back in.");
    overlay.setPinned(true, t0 + 2500ms);
    ok &= check(
        overlay.isPinned() &&
        overlay.wantsVisible(t0 + 10s),
        "A pinned overlay must remain visible.");
    overlay.setFullScreen(false, t0 + 11s);
    ok &= check(
        !overlay.isFullScreen() &&
        !overlay.isPinned(),
        "Leaving full-screen must clear the pin.");

    ok &= check(
        std::abs(getPlaybackProgress(50.0, 0.0, 100.0) - .5) < .0001,
        "Playback progress must be normalized.");
    ok &= check(
        0.0 == getPlaybackProgress(-20.0, 0.0, 100.0) &&
        1.0 == getPlaybackProgress(120.0, 0.0, 100.0) &&
        0.0 == getPlaybackProgress(10.0, 0.0, 0.0),
        "Playback progress must be clamped and reject invalid duration.");

    return ok ? 0 : 1;
}
