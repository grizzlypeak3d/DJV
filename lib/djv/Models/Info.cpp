// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/Models/Info.h>

#include <ftk/Core/String.h>

#include <sstream>

namespace djv
{
    namespace models
    {
        namespace
        {
            template<typename T>
            std::string str(const T& value)
            {
                std::stringstream ss;
                ss << value;
                return ss.str();
            }

            std::string timecode(const OTIO_NS::RationalTime& value)
            {
                std::stringstream ss;
                ss << value.to_timecode();
                return ss.str();
            }

            //! Audio times are counted in samples, at a rate that is not a
            //! timecode rate, so they are given in seconds.
            std::string seconds(const OTIO_NS::RationalTime& value)
            {
                std::stringstream ss;
                ss.precision(2);
                ss << std::fixed << value.rescaled_to(1.0).value() << " seconds";
                return ss.str();
            }

            std::string rate(double value)
            {
                std::stringstream ss;
                ss.precision(1);
                ss << std::fixed << value / 1000.0 << "kHz";
                return ss.str();
            }
        }

        const std::vector<std::string>& getInfoSections()
        {
            static const std::vector<std::string> out =
            {
                "File", "Video", "Audio", "Metadata"
            };
            return out;
        }

        std::vector<InfoItem> getInfo(
            const ftk::Path& path,
            const tl::IOInfo& info,
            int videoLayer)
        {
            std::vector<InfoItem> out;

            // Name and directory rather than one full path: the name is
            // what somebody scanning the tool wants first, and the two
            // compose the full path for sharing.
            if (!path.isEmpty())
            {
                out.push_back({ "File", "Name", path.getFileName() });
                out.push_back({ "File", "Directory", path.getDir() });
                if (path.isSeq())
                {
                    out.push_back({ "File", "Frames", path.getFrameRange(true) });
                }
            }

            // The video the file holds, and the video it is decoded to. The
            // two are not always the same and the difference is the point:
            // what the rest of the application reports is the decoded one.
            if (!info.video.empty())
            {
                const ftk::ImageInfo& video = tl::getVideoInfo(info, videoLayer);
                if (!info.videoSource.codec.empty())
                {
                    out.push_back({ "Video", "Codec", info.videoSource.codec });
                }
                if (!info.videoSource.pixelFormat.empty())
                {
                    out.push_back({ "Video", "Source Format", info.videoSource.pixelFormat });
                }
                out.push_back({ "Video", "Resolution",
                    str(video.size.w) + " " + str(video.size.h) });
                // The interleaved name: whether this renderer holds the
                // channels in planes is not something the file said.
                out.push_back({ "Video", "Pixel Type",
                    str(ftk::getInterleavedType(video.type)) });
                {
                    std::stringstream ss;
                    ss.precision(2);
                    ss << std::fixed << video.pixelAspectRatio;
                    out.push_back({ "Video", "Pixel Aspect Ratio", ss.str() });
                }
                out.push_back({ "Video", "Levels", str(video.videoLevels) });
                if (info.videoTime.has_value())
                {
                    out.push_back({ "Video", "Start Time", timecode(info.videoTime->start_time()) });
                    out.push_back({ "Video", "Duration", timecode(info.videoTime->duration()) });
                    std::stringstream ss;
                    ss.precision(2);
                    ss << std::fixed << info.videoTime->start_time().rate() << " FPS";
                    out.push_back({ "Video", "Speed", ss.str() });
                }
            }

            if (info.audio.isValid())
            {
                const tl::AudioSourceInfo& source = info.audioSource;
                if (!source.codec.empty())
                {
                    out.push_back({ "Audio", "Codec", source.codec });
                }
                out.push_back({ "Audio", "Channels", str(info.audio.channelCount) });
                out.push_back({ "Audio", "Type", str(info.audio.type) });
                out.push_back({ "Audio", "Sample Rate", rate(info.audio.sampleRate) });
                // Only when the file is not what comes out of the reader,
                // which is the case worth showing.
                if (source.channelCount != 0 &&
                    static_cast<int>(source.channelCount) != info.audio.channelCount)
                {
                    out.push_back({ "Audio", "Source Channels",
                        str(static_cast<int>(source.channelCount)) });
                }
                if (source.type != tl::AudioType::None &&
                    source.type != info.audio.type)
                {
                    out.push_back({ "Audio", "Source Type", str(source.type) });
                }
                if (source.sampleRate != 0 &&
                    static_cast<int>(source.sampleRate) != info.audio.sampleRate)
                {
                    out.push_back({ "Audio", "Source Sample Rate", rate(source.sampleRate) });
                }
                if (info.audioTime.has_value())
                {
                    out.push_back({ "Audio", "Start Time", seconds(info.audioTime->start_time()) });
                    out.push_back({ "Audio", "Duration", seconds(info.audioTime->duration()) });
                }
            }

            for (const auto& tag : info.tags)
            {
                out.push_back({ "Metadata", tag.first, tag.second });
            }
            return out;
        }

        std::vector<InfoItem> filterInfo(
            const std::vector<InfoItem>& items,
            const std::string& search)
        {
            const std::vector<std::string> words = ftk::split(search, { ' ', '\t' });
            if (words.empty())
                return items;
            std::vector<InfoItem> out;
            for (const auto& item : items)
            {
                for (const auto& word : words)
                {
                    if (ftk::contains(item.name, word, ftk::CaseCompare::Insensitive) ||
                        ftk::contains(item.value, word, ftk::CaseCompare::Insensitive))
                    {
                        out.push_back(item);
                        break;
                    }
                }
            }
            return out;
        }
    }
}
