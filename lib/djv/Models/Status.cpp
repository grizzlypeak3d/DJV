// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/Models/Status.h>

namespace djv
{
    namespace models
    {
        std::vector<StatusItem> getStatusItems()
        {
            return
            {
                { "Channels", "Image channels" },
                { "Negative", "Negative" },
                { "ClippingWarning", "Clipping warning" },
                { "Mirror", "Mirror" },
                { "AspectRatio", "Aspect ratio" },
                { "OCIO", "OCIO" },
                { "LUT", "LUT" },
                { "Color", "Color controls" },
                { "AudioOffset", "Audio offset" },
                { "AudioChannel", "Audio channel" }
            };
        }

        std::map<std::string, bool> getStatus(const StatusOptions& options)
        {
            const tl::DisplayOptions& display = options.display;
            // Only an adjustment that changes the picture: enabled at its
            // defaults it is neutral, and the renderer skips it or nearly
            // so, so there is nothing to be reminded of.
            tl::Color color = display.color;
            color.enabled = false;
            tl::Levels levels = display.levels;
            levels.enabled = false;
            return
            {
                { "Channels", display.channels != ftk::ChannelDisplay::Color },
                { "Negative", display.negative },
                { "ClippingWarning", options.foreground.clippingWarning.enabled },
                { "Mirror", display.mirror.x || display.mirror.y },
                { "AspectRatio", options.aspectRatio.index != 0 },
                { "OCIO",
                    options.ocio.enabled &&
                    !options.ocio.display.empty() &&
                    !options.ocio.view.empty() },
                { "LUT", options.lut.enabled && !options.lut.fileName.empty() },
                { "Color",
                    (display.color.enabled && color != tl::Color()) ||
                    (display.levels.enabled && levels != tl::Levels()) ||
                    (display.exposure.enabled && display.exposure.exposure != 0.F) ||
                    (display.softClip.enabled && display.softClip.value > 0.F) },
                { "AudioOffset", options.audioOffset != 0.0 },
                { "AudioChannel", options.audioChannel != -1 }
            };
        }
    }
}
