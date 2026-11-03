// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/Models/ViewportModel.h>

#include <tlRender/Timeline/ColorOptions.h>
#include <tlRender/Timeline/DisplayOptions.h>
#include <tlRender/Timeline/Player.h>
#include <tlRender/IO/IO.h>

#include <ftk/Core/Path.h>

#include <optional>
#include <string>

namespace djv
{
    namespace models
    {
        //! What the HUD's lines are about: the file, the playback, and how
        //! the picture is drawn. The time label is in the user's units.
        struct DJV_MODELS_API_TYPE HUDData
        {
            ftk::Path path;
            tl::IOInfo ioInfo;
            int videoLayer = 0;
            std::optional<OTIO_NS::RationalTime> currentTime;
            std::string timeLabel;
            double fps = 0.0;
            size_t droppedFrames = 0;
            //! A frame missing from the media, and the frame standing in.
            bool missing = false;
            std::optional<int64_t> heldFrom;
            tl::PlayerCacheInfo cacheInfo;
            tl::AspectRatioOptions aspectRatio;
            tl::LUTOptions lutOptions;
            double viewZoom = 1.0;
        };

        //! The HUD line for an item, as the desktop and the iPad show it;
        //! empty when the item has nothing to say about what is open. The
        //! color picker is drawn by the viewport, and has no line here.
        DJV_MODELS_API std::string getHUDText(HUDItem, const HUDData&);
    }
}
