// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/Models/Info.h>

#include <ftk/Core/Format.h>
#include <ftk/Core/String.h>

#include <algorithm>
#include <cctype>
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
                "File", "Header", "Video", "Audio", "Metadata"
            };
            return out;
        }

        namespace
        {
            //! The header tags, in the order they are shown.
            const std::vector<std::string>& getHeaderKeys()
            {
                static const std::vector<std::string> out =
                {
                    "Name",
                    "Type",
                    "View",
                    "Data Window",
                    "Display Window",
                    "Channels",
                    "Compression",
                    "Tile"
                };
                return out;
            }

            bool isHeaderKey(const std::string& key)
            {
                const auto& keys = getHeaderKeys();
                return std::find(keys.begin(), keys.end(), key) != keys.end();
            }

            //! A window, "x0 y0 x1 y1", with its size after it.
            std::string window(const std::string& value)
            {
                std::stringstream ss(value);
                int x0 = 0;
                int y0 = 0;
                int x1 = 0;
                int y1 = 0;
                if (!(ss >> x0 >> y0 >> x1 >> y1))
                {
                    return value;
                }
                return ftk::Format("{0} ({1} x {2})").
                    arg(value).
                    arg(x1 - x0 + 1).
                    arg(y1 - y0 + 1);
            }

            std::string headerValue(const std::string& key, const std::string& value)
            {
                return "Data Window" == key || "Display Window" == key ?
                    window(value) :
                    value;
            }

            //! The part a tag is of, "Part N/", and the tag without it.
            bool splitPart(const std::string& key, int& part, std::string& name)
            {
                if (0 != key.compare(0, 5, "Part "))
                {
                    return false;
                }
                const size_t slash = key.find('/', 5);
                if (std::string::npos == slash || 5 == slash)
                {
                    return false;
                }
                for (size_t i = 5; i < slash; ++i)
                {
                    if (!std::isdigit(static_cast<unsigned char>(key[i])))
                    {
                        return false;
                    }
                }
                part = std::atoi(key.substr(5, slash - 5).c_str());
                name = key.substr(slash + 1);
                return true;
            }
        }

        ftk::ImageTags getFrameTags(const std::vector<tl::VideoFrame>& frames)
        {
            ftk::ImageTags out;
            if (!frames.empty() &&
                !frames.front().layers.empty() &&
                frames.front().layers.front().image)
            {
                out = frames.front().layers.front().image->getTags();
            }
            return out;
        }

        std::vector<InfoItem> getInfo(
            const ftk::Path& path,
            const tl::IOInfo& info,
            int videoLayer,
            const ftk::ImageTags& frameTags)
        {
            std::vector<InfoItem> out;
            // The frame's tags, and the audio's: the audio is not part of
            // the frame.
            ftk::ImageTags tags = !frameTags.empty() ? frameTags : info.tags;
            for (const auto& tag : info.tags)
            {
                if (0 == tag.first.compare(0, 6, "Audio/"))
                {
                    tags[tag.first] = tag.second;
                }
            }

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
                    // Which frames a broken render is missing, for whoever
                    // has to go and find them.
                    const auto missing = ftk::getMissing(path.getSeq());
                    if (!missing.empty())
                    {
                        out.push_back({ "File", "Missing",
                            ftk::getLabel(missing, path.getPad()) });
                    }
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

            // The header, given by the OpenEXR reader: the first part's tags,
            // and for a file of more than one part, each other part's.
            std::vector<InfoItem> header;
            const bool hasHeader =
                tags.find("Data Window") != tags.end() ||
                tags.find("Parts") != tags.end();
            int parts = 1;
            if (const auto i = tags.find("Parts"); i != tags.end())
            {
                parts = std::max(1, std::atoi(i->second.c_str()));
            }
            if (hasHeader)
            {
                int shown = -1;
                if (const auto i = tags.find("Part"); i != tags.end())
                {
                    shown = std::atoi(i->second.c_str());
                }
                if (parts > 1)
                {
                    header.push_back({ "Header", "Parts", str(parts) });
                }
                for (int part = 0; part < parts; ++part)
                {
                    const std::string prefix = part > 0 ?
                        std::string(ftk::Format("Part {0}/").arg(part)) :
                        std::string();
                    const std::string indent = parts > 1 ? "  " : "";
                    if (parts > 1)
                    {
                        std::string value;
                        if (const auto i = tags.find(prefix + "Name"); i != tags.end())
                        {
                            value = i->second;
                        }
                        if (part == shown)
                        {
                            value += value.empty() ? "(shown)" : " (shown)";
                        }
                        header.push_back({ "Header", ftk::Format("Part {0}").arg(part), value });
                    }
                    for (const auto& key : getHeaderKeys())
                    {
                        // The name is the part's row when there are parts.
                        if (parts > 1 && "Name" == key)
                            continue;
                        const auto i = tags.find(prefix + key);
                        if (i != tags.end())
                        {
                            header.push_back({ "Header", indent + key, headerValue(key, i->second) });
                        }
                    }
                }
            }
            out.insert(out.end(), header.begin(), header.end());
            // In section order: the header goes after the file.
            std::stable_partition(
                out.begin(),
                out.end(),
                [](const InfoItem& item)
                {
                    return "File" == item.section || "Header" == item.section;
                });

            // The reader's details of the file and its streams go with
            // the file, the video, and the audio, after what is there.
            std::vector<InfoItem> details;
            for (const auto& tag : tags)
            {
                for (const auto& section : { "File", "Video", "Audio" })
                {
                    const std::string prefix = std::string(section) + "/";
                    if (0 == tag.first.compare(0, prefix.size(), prefix))
                    {
                        details.push_back({ section, tag.first.substr(prefix.size()), tag.second });
                    }
                }
                if ("Color Primaries" == tag.first ||
                    "Color Transfer" == tag.first ||
                    "Color Matrix" == tag.first)
                {
                    details.push_back({ "Video", tag.first, tag.second });
                }
            }
            for (const auto& item : details)
            {
                auto i = std::find_if(
                    out.rbegin(),
                    out.rend(),
                    [&item](const InfoItem& value)
                    {
                        return value.section == item.section;
                    });
                if (i != out.rend())
                {
                    out.insert(i.base(), item);
                }
                else
                {
                    // A section with nothing else in it goes in its place.
                    const auto& sections = getInfoSections();
                    const auto s = std::find(sections.begin(), sections.end(), item.section);
                    auto j = std::find_if(
                        out.begin(),
                        out.end(),
                        [&](const InfoItem& value)
                        {
                            return std::find(sections.begin(), sections.end(), value.section) > s;
                        });
                    out.insert(j, item);
                }
            }

            for (const auto& tag : tags)
            {
                // Given above, or the reader's own: the HDR metadata of a
                // movie's frame, for the renderer.
                if (tl::isSourceTag(tag.first) ||
                    "Color Primaries" == tag.first ||
                    "Color Transfer" == tag.first ||
                    "Color Matrix" == tag.first ||
                    "hdr" == tag.first)
                    continue;
                if (hasHeader)
                {
                    if (isHeaderKey(tag.first) ||
                        "Parts" == tag.first ||
                        "Part" == tag.first)
                        continue;
                    int part = 0;
                    std::string name;
                    if (splitPart(tag.first, part, name) && isHeaderKey(name))
                        continue;
                }
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
