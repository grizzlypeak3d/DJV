// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/Models/Export.h>

#include <string>
#include <vector>

namespace djv
{
    namespace models
    {
        //! \name Settings Keys
        ///@{

        //! Every key the application keeps settings under, in one place.
        //!
        //! The settings are one JSON document and a key is a path into it,
        //! so a key written as a whole takes everything under it along: a
        //! group written to "/Review" took the recent reviews kept at
        //! "/Review/Recent" with it on every exit. Declaring the keys here,
        //! with a test that no key lies under another, is what keeps that
        //! from happening again. A number on the end of a group's key is
        //! its version: a group whose layout changed takes a new number, and
        //! what was under the old one is left alone.
        namespace settingsKeys
        {
            //! SettingsModel groups: a whole object each, written as one.
            constexpr const char* audio = "/Audio.1";
            constexpr const char* cache = "/Cache";
            constexpr const char* thumbnailCache = "/ThumbnailCache";
            constexpr const char* exportSettings = "/Export";
            constexpr const char* fileBrowser = "/FileBrowser";
            constexpr const char* imageSeq = "/ImageSeq.1";
            constexpr const char* otio = "/OTIO.2";
            constexpr const char* shortcuts = "/Shortcuts.4";
            //! Read for the shortcuts of a version before, not written.
            constexpr const char* shortcutsPrevious = "/Shortcuts.3";
            constexpr const char* misc = "/Misc.3";
            constexpr const char* mouse = "/Mouse.1";
            constexpr const char* playback = "/Playback.1";
            constexpr const char* style = "/Style.2";
            constexpr const char* timeline = "/Timeline";
            constexpr const char* window = "/Window";
            constexpr const char* ffmpeg = "/FFmpeg";
            constexpr const char* ffmpegCmd = "/FFmpegCmd";
            constexpr const char* usd = "/USD.1";
            //! The models' own values.
            constexpr const char* audioVolume = "/Audio/Volume";
            constexpr const char* audioMute = "/Audio/Mute";
            constexpr const char* colorOCIO = "/Color/OCIO";
            constexpr const char* colorOCIOExtColorSpaces = "/Color/OCIOExtColorSpaces";
            constexpr const char* colorLUT = "/Color/LUT";
            constexpr const char* colorLevelsInRange = "/Color/Levels/InRange";
            constexpr const char* colorLevelsOutRange = "/Color/Levels/OutRange";
            constexpr const char* colorWidgetMode = "/ColorWidget/Mode";
            constexpr const char* drawColor = "/Draw/Color";
            constexpr const char* drawSize = "/Draw/Size";
            constexpr const char* filesCompareDifferenceGain = "/Files/Compare/DifferenceGain";
            constexpr const char* filesCompareOverlay = "/Files/Compare/Overlay";
            constexpr const char* filesCompareSameSize = "/Files/Compare/SameSize";
            constexpr const char* filesCompareTime = "/Files/Compare/Time";
            constexpr const char* filesCompareWipeCenter = "/Files/Compare/WipeCenter";
            constexpr const char* filesCompareWipeRotation = "/Files/Compare/WipeRotation";
            constexpr const char* informationBellows = "/Information/Bellows";
            constexpr const char* magnifyLevel = "/Magnify/Level";
            constexpr const char* magnifyViewPosAndZoom = "/Magnify/ViewPosAndZoom";
            constexpr const char* timeUnits = "/TimeUnits";
            constexpr const char* toolsOpen = "/Tools/Open.1";
            constexpr const char* viewportAspectRatio = "/Viewport/AspectRatio.1";
            constexpr const char* viewportBackground = "/Viewport/Background";
            constexpr const char* viewportColorBuffer = "/Viewport/ColorBuffer";
            constexpr const char* viewportDisplay = "/Viewport/Display";
            constexpr const char* viewportForeground = "/Viewport/Foreground.1";
            constexpr const char* viewportHDRTransfer = "/Viewport/HDRTransfer";
            constexpr const char* viewportHDRWhite = "/Viewport/HDRWhite";
            constexpr const char* viewportHUD = "/Viewport/HUD.2";
            constexpr const char* viewportImage = "/Viewport/Image.1";

            //! The recent lists, kept by RecentFilesModel under a group of
            //! their own, "/<group>/Recent" and "/<group>/RecentMax", so a
            //! group written whole never takes a list with it.
            constexpr const char* recentFilesGroup = "Files";
            constexpr const char* recentReviewsGroup = "Review";
            constexpr const char* recentPlaylistsGroup = "Playlist";
            constexpr const char* recentDirsGroup = "FileBrowserDirs";

            //! The keys of a recent list, from its group.
            DJV_MODELS_API std::string recent(const std::string& group);
            DJV_MODELS_API std::string recentMax(const std::string& group);
        }

        //! Every key, for the test that none lies under another.
        DJV_MODELS_API const std::vector<std::string>& getSettingsKeys();

        ///@}
    }
}
