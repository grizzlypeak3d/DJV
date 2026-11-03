// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/Models/HUD.h>

#include <ftk/Core/Format.h>
#include <ftk/Core/String.h>

#include <cmath>
#include <vector>

namespace djv
{
    namespace models
    {
        std::string getHUDText(HUDItem item, const HUDData& data)
        {
            std::string out;
            switch (item)
            {
            case HUDItem::FileName:
            {
                // For a sequence, the image on screen rather than the
                // pattern: which frame a picture came from is the question a
                // sequence of renders gets asked. Sequence time is the frame
                // number, so the value is the name (#490).
                out =
                    data.path.isSeq() && data.currentTime.has_value() ?
                    data.path.getFrame(static_cast<int64_t>(data.currentTime->value())) :
                    data.path.getFileName();
                if (out.empty())
                {
                    out = "(No file)";
                }
                break;
            }
            case HUDItem::Info:
            {
                std::vector<std::string> info;
                if (!data.ioInfo.video.empty())
                {
                    info.push_back(std::string(ftk::Format("V: {0}").
                        arg(ftk::getLabel(
                            tl::getVideoInfo(data.ioInfo, data.videoLayer)))));
                }
                if (data.ioInfo.audio.isValid())
                {
                    info.push_back(std::string(ftk::Format("A: {0}").
                        arg(tl::getLabel(data.ioInfo.audio, true))));
                }
                out = ftk::join(info, ", ");
                break;
            }
            case HUDItem::Render:
            {
                // What is actually rendered, which the pixel aspect ratio and
                // the aspect ratio override can both move away from the media
                // size reported above. The effective pixel aspect ratio is
                // taken back out of the render size rather than read from
                // either source, so it holds whether it came from the media
                // or from an override.
                if (!data.ioInfo.video.empty())
                {
                    const ftk::ImageInfo& videoInfo = data.ioInfo.video[0];
                    const ftk::Size2I renderSize = tl::getRenderSize(
                        videoInfo,
                        data.aspectRatio);
                    if (renderSize.isValid() && videoInfo.size.w > 0)
                    {
                        const float pixelAspectRatio =
                            renderSize.w / static_cast<float>(videoInfo.size.w);
                        out = ftk::Format("Render: {0}x{1}:{2}").
                            arg(renderSize.w).
                            arg(renderSize.h).
                            arg(ftk::aspectRatio(renderSize), 2);
                        // Square pixels are the common case and add nothing.
                        if (std::fabs(pixelAspectRatio - 1.F) > 0.001F)
                        {
                            out += ftk::Format(", PAR: {0}").
                                arg(pixelAspectRatio, 2);
                        }
                    }
                }
                break;
            }
            case HUDItem::LUT:
                // Which LUT is on, by its file name: two that look alike are
                // otherwise only told apart in the color tool (#921).
                out = ftk::Format("LUT: {0}").arg(
                    data.lutOptions.enabled && !data.lutOptions.fileName.empty() ?
                    ftk::Path(data.lutOptions.fileName).getFileName() :
                    std::string("-"));
                break;
            case HUDItem::Time:
            {
                std::string missing;
                if (data.missing)
                {
                    // Said rather than only drawn, so which frame is standing
                    // in is known and not merely that one is.
                    missing = data.heldFrom.has_value() ?
                        ftk::Format(", held from {0}").
                            arg(data.heldFrom.value()).str() :
                        ", missing";
                }
                // Frames per second and frames dropped are about video, and a
                // file without any has neither -- reporting none and an
                // ever-growing count of what was never going to arrive reads
                // as a fault rather than as an absence.
                out = ftk::Format("Time: {0}{1}").
                    arg(data.timeLabel).
                    arg(missing);
                if (!data.ioInfo.video.empty())
                {
                    out = ftk::Format("Time: {0}, {1} FPS, {2} dropped{3}").
                        arg(data.timeLabel).
                        arg(data.fps, 2, 6).
                        arg(static_cast<int>(data.droppedFrames), 3).
                        arg(missing);
                }
                break;
            }
            case HUDItem::ViewZoom:
                out = ftk::Format("Zoom: {0}").arg(data.viewZoom, 2, 6);
                break;
            case HUDItem::Cache:
            {
                std::vector<std::string> cache;
                if (!data.ioInfo.video.empty())
                {
                    cache.push_back(std::string(ftk::Format("{0}% V").
                        arg(static_cast<int>(data.cacheInfo.videoPercentage), 3)));
                }
                if (data.ioInfo.audio.isValid())
                {
                    cache.push_back(std::string(ftk::Format("{0}% A").
                        arg(static_cast<int>(data.cacheInfo.audioPercentage), 3)));
                }
                if (!cache.empty())
                {
                    out = ftk::Format("Cache: {0}").arg(ftk::join(cache, ", "));
                }
                break;
            }
            default: break;
            }
            return out;
        }
    }
}
