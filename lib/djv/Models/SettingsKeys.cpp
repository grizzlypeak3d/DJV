// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/Models/SettingsKeys.h>

namespace djv
{
    namespace models
    {
        namespace settingsKeys
        {
            std::string recent(const std::string& group)
            {
                return "/" + group + "/Recent";
            }

            std::string recentMax(const std::string& group)
            {
                return "/" + group + "/RecentMax";
            }
        }

        const std::vector<std::string>& getSettingsKeys()
        {
            static const std::vector<std::string> keys =
            {
                settingsKeys::audio,
                settingsKeys::cache,
                settingsKeys::thumbnailCache,
                settingsKeys::exportSettings,
                settingsKeys::fileBrowser,
                settingsKeys::imageSeq,
                settingsKeys::otio,
                settingsKeys::shortcuts,
                settingsKeys::shortcutsPrevious,
                settingsKeys::misc,
                settingsKeys::mouse,
                settingsKeys::reviewSettings,
                settingsKeys::playback,
                settingsKeys::style,
                settingsKeys::timeline,
                settingsKeys::window,
                settingsKeys::ffmpeg,
                settingsKeys::ffmpegCmd,
                settingsKeys::usd,
                settingsKeys::audioVolume,
                settingsKeys::audioMute,
                settingsKeys::colorOCIO,
                settingsKeys::colorOCIOExtColorSpaces,
                settingsKeys::colorLUT,
                settingsKeys::colorLevelsInRange,
                settingsKeys::colorLevelsOutRange,
                settingsKeys::colorWidgetMode,
                settingsKeys::drawColor,
                settingsKeys::drawSize,
                settingsKeys::drawTextSize,
                settingsKeys::drawOnionSkin,
                settingsKeys::filesCompareDifferenceGain,
                settingsKeys::filesCompareOverlay,
                settingsKeys::filesCompareSameSize,
                settingsKeys::filesCompareTime,
                settingsKeys::filesCompareWipeCenter,
                settingsKeys::filesCompareWipeRotation,
                settingsKeys::informationBellows,
                settingsKeys::magnifyLevel,
                settingsKeys::magnifyViewPosAndZoom,
                settingsKeys::timeUnits,
                settingsKeys::toolsOpen,
                settingsKeys::viewportAspectRatio,
                settingsKeys::viewportBackground,
                settingsKeys::viewportColorBuffer,
                settingsKeys::viewportDisplay,
                settingsKeys::viewportForeground,
                settingsKeys::viewportHUD,
                settingsKeys::viewportImage,
                settingsKeys::recent(settingsKeys::recentFilesGroup),
                settingsKeys::recentMax(settingsKeys::recentFilesGroup),
                settingsKeys::recent(settingsKeys::recentReviewsGroup),
                settingsKeys::recentMax(settingsKeys::recentReviewsGroup),
                settingsKeys::recent(settingsKeys::recentPlaylistsGroup),
                settingsKeys::recentMax(settingsKeys::recentPlaylistsGroup),
                settingsKeys::recent(settingsKeys::recentDirsGroup),
                settingsKeys::recentMax(settingsKeys::recentDirsGroup),
            };
            return keys;
        }
    }
}
