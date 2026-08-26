// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/Models/Playlist.h>

#include <tlRender/Timeline/Util.h>

#include <ftk/Core/Format.h>

#include <opentimelineio/clip.h>
#include <opentimelineio/externalReference.h>
#include <opentimelineio/gap.h>
#include <opentimelineio/imageSequenceReference.h>
#include <opentimelineio/stack.h>
#include <opentimelineio/stackAlgorithm.h>
#include <opentimelineio/track.h>
#include <opentimelineio/transition.h>

#include <filesystem>

namespace djv
{
    namespace models
    {
        namespace
        {
            const int64_t playlistVersion = 1;

            std::string genericPath(const std::filesystem::path& value)
            {
                return value.generic_u8string();
            }

            // The URL for a file as a playlist stores it: relative to the
            // playlist's directory when the file is under it, so a playlist
            // saved above its media moves with it, and absolute otherwise.
            // Lexically everything is reachable by enough "..", but a chain
            // of them pins the playlist to where it was saved from, which is
            // what a relative path is meant to avoid.
            std::string fileUrl(
                const std::string& fileName,
                const std::string& directory)
            {
                std::error_code ec;
                const std::filesystem::path path = std::filesystem::absolute(
                    std::filesystem::u8path(fileName), ec);
                if (ec)
                {
                    return genericPath(std::filesystem::u8path(fileName));
                }
                std::string out = genericPath(path);
                if (!directory.empty())
                {
                    const std::filesystem::path relative = std::filesystem::proximate(
                        path, std::filesystem::u8path(directory), ec);
                    if (!ec &&
                        !relative.empty() &&
                        relative.begin()->u8string() != "..")
                    {
                        out = genericPath(relative);
                    }
                }
                return out;
            }

            template<typename T>
            T getMeta(
                const OTIO_NS::AnyDictionary& dict,
                const std::string& key,
                const T& defaultValue)
            {
                T out = defaultValue;
                const auto i = dict.find(key);
                if (i != dict.end() && i->second.type() == typeid(T))
                {
                    out = std::any_cast<T>(i->second);
                }
                return out;
            }

            // JSON has one kind of number and two homes for it, so a value
            // written by hand as "24" arrives as an integer even where a
            // double is meant.
            double getMetaDouble(
                const OTIO_NS::AnyDictionary& dict,
                const std::string& key,
                double defaultValue)
            {
                double out = getMeta(dict, key, defaultValue);
                const auto i = dict.find(key);
                if (i != dict.end() && i->second.type() == typeid(int64_t))
                {
                    out = static_cast<double>(std::any_cast<int64_t>(i->second));
                }
                return out;
            }

            OTIO_NS::AnyDictionary toAny(const tl::CompareOptions& value)
            {
                OTIO_NS::AnyDictionary out;
                out["compare"] = to_string(value.compare);
                OTIO_NS::AnyVector wipeCenter;
                wipeCenter.push_back(static_cast<double>(value.wipeCenter.x));
                wipeCenter.push_back(static_cast<double>(value.wipeCenter.y));
                out["wipeCenter"] = wipeCenter;
                out["wipeRotation"] = static_cast<double>(value.wipeRotation);
                out["overlay"] = static_cast<double>(value.overlay);
                out["differenceGain"] = static_cast<double>(value.differenceGain);
                out["sameSize"] = value.sameSize;
                return out;
            }

            tl::CompareOptions compareFromAny(const OTIO_NS::AnyDictionary& dict)
            {
                tl::CompareOptions out;
                tl::Compare compare = tl::Compare::None;
                if (from_string(getMeta(dict, "compare", std::string()), compare))
                {
                    out.compare = compare;
                }
                const auto wipeCenter = getMeta(dict, "wipeCenter", OTIO_NS::AnyVector());
                if (2 == wipeCenter.size() &&
                    wipeCenter[0].type() == typeid(double) &&
                    wipeCenter[1].type() == typeid(double))
                {
                    out.wipeCenter.x = static_cast<float>(std::any_cast<double>(wipeCenter[0]));
                    out.wipeCenter.y = static_cast<float>(std::any_cast<double>(wipeCenter[1]));
                }
                out.wipeRotation = static_cast<float>(
                    getMetaDouble(dict, "wipeRotation", out.wipeRotation));
                out.overlay = static_cast<float>(
                    getMetaDouble(dict, "overlay", out.overlay));
                out.differenceGain = static_cast<float>(
                    getMetaDouble(dict, "differenceGain", out.differenceGain));
                out.sameSize = getMeta(dict, "sameSize", out.sameSize);
                return out;
            }
        }

        OTIO_NS::SerializableObject::Retainer<OTIO_NS::Timeline> playlistToOTIO(
            const Playlist& playlist,
            const std::string& directory,
            double defaultRate)
        {
            auto track = new OTIO_NS::Track(
                "Video", std::nullopt, OTIO_NS::Track::Kind::video);
            for (const auto& item : playlist.items)
            {
                // The whole file name: the base alone loses whatever looked
                // like a frame number, so "Dinky_2015-06-11.m4v" would be
                // named "Dinky_2015-06".
                auto clip = new OTIO_NS::Clip(item->path.getFileName());

                if (item->path.isSeq())
                {
                    // A sequence that has been opened knows its rate; one
                    // that has not takes the default, the same way it would
                    // when opened.
                    double rate = defaultRate;
                    if (item->timeRange.has_value())
                    {
                        rate = item->timeRange->duration().rate();
                    }
                    std::optional<ftk::RangeI64> frames = item->path.getFrames();
                    if (!frames.has_value() && item->timeRange.has_value())
                    {
                        frames = ftk::RangeI64(
                            static_cast<int64_t>(item->timeRange->start_time().value()),
                            static_cast<int64_t>(item->timeRange->end_time_inclusive().value()));
                    }
                    const int64_t startFrame = frames.has_value() ? frames->min() : 0;
                    auto ref = new OTIO_NS::ImageSequenceReference(
                        fileUrl(item->path.getDir(), directory) + "/",
                        item->path.getBase(),
                        item->path.getExt(),
                        startFrame,
                        1,
                        rate,
                        item->path.getPad());
                    if (frames.has_value())
                    {
                        ref->set_available_range(OTIO_NS::TimeRange(
                            OTIO_NS::RationalTime(static_cast<double>(frames->min()), rate),
                            OTIO_NS::RationalTime(
                                static_cast<double>(frames->max() - frames->min() + 1), rate)));
                    }
                    clip->set_media_reference(ref);
                }
                else
                {
                    clip->set_media_reference(new OTIO_NS::ExternalReference(
                        fileUrl(item->path.getFileName(true), directory)));
                }

                if (item->inOutRange.has_value())
                {
                    clip->set_source_range(item->inOutRange);
                }

                // The item's own metadata first, so what another application
                // put on the clip this item came from goes back out with it.
                OTIO_NS::AnyDictionary metadata = item->metadata;
                OTIO_NS::AnyDictionary djv;
                if (!item->audioPath.isEmpty())
                {
                    // Absolute, for the same reason the timeline gives its
                    // separate audio an absolute reference: the audio was
                    // chosen from somewhere else, and a relative reference
                    // would look for it beside the playlist instead.
                    std::error_code ec;
                    const std::filesystem::path audioPath = std::filesystem::absolute(
                        std::filesystem::u8path(item->audioPath.getFileName(true)), ec);
                    djv["audioPath"] = ec ?
                        genericPath(std::filesystem::u8path(item->audioPath.getFileName(true))) :
                        genericPath(audioPath);
                }
                if (item->videoLayer != 0)
                {
                    djv["videoLayer"] = static_cast<int64_t>(item->videoLayer);
                }
                if (item->speed > 0.0)
                {
                    djv["speed"] = item->speed;
                }
                if (item->currentTime.has_value())
                {
                    djv["currentTime"] = *item->currentTime;
                }
                if (item->path.isSeq() && item->framesStated)
                {
                    djv["framesStated"] = true;
                }
                metadata["djv"] = djv;
                clip->metadata() = metadata;

                track->append_child(clip);
            }

            auto stack = new OTIO_NS::Stack;
            stack->append_child(track);
            auto timeline = new OTIO_NS::Timeline;
            timeline->set_tracks(stack);

            OTIO_NS::AnyDictionary djv;
            djv["playlist"] = playlistVersion;
            if (playlist.aIndex >= 0)
            {
                djv["aIndex"] = static_cast<int64_t>(playlist.aIndex);
            }
            if (!playlist.bIndexes.empty())
            {
                OTIO_NS::AnyVector bIndexes;
                for (int i : playlist.bIndexes)
                {
                    bIndexes.push_back(static_cast<int64_t>(i));
                }
                djv["bIndexes"] = bIndexes;
            }
            if (playlist.compareOptions != tl::CompareOptions())
            {
                djv["compareOptions"] = toAny(playlist.compareOptions);
            }
            if (playlist.compareTime != tl::CompareTime::Relative)
            {
                djv["compareTime"] = to_string(playlist.compareTime);
            }
            timeline->metadata()["djv"] = djv;

            return OTIO_NS::SerializableObject::Retainer<OTIO_NS::Timeline>(timeline);
        }

        Playlist playlistFromOTIO(
            const OTIO_NS::SerializableObject::Retainer<OTIO_NS::Timeline>& timeline,
            const std::string& directory,
            std::vector<std::string>& report)
        {
            Playlist out;

            const auto djvT = getMeta(
                timeline->metadata(), "djv", OTIO_NS::AnyDictionary());
            const int64_t version = getMeta(djvT, "playlist", int64_t(0));
            if (version > playlistVersion)
            {
                // Reading part of what a newer DJV wrote and saving the rest
                // away would corrupt the document, so refuse instead.
                throw std::runtime_error(ftk::Format(
                    "The playlist was written by a newer version of DJV: version {0}").
                    arg(version));
            }

            // Gather the tracks, flattening multiple video tracks into one.
            // The source file is never rewritten, so nothing is lost on disk;
            // what the file list cannot carry is counted and reported below.
            std::vector<OTIO_NS::Track*> videoTracks;
            size_t audioTracks = 0;
            for (const auto& child : timeline->tracks()->children())
            {
                if (auto track = OTIO_NS::dynamic_retainer_cast<OTIO_NS::Track>(child))
                {
                    if (OTIO_NS::Track::Kind::video == track->kind())
                    {
                        videoTracks.push_back(track);
                    }
                    else if (OTIO_NS::Track::Kind::audio == track->kind())
                    {
                        ++audioTracks;
                    }
                }
            }
            OTIO_NS::SerializableObject::Retainer<OTIO_NS::Track> flattened;
            OTIO_NS::Track* track = nullptr;
            if (videoTracks.size() > 1)
            {
                OTIO_NS::ErrorStatus errorStatus;
                flattened = OTIO_NS::flatten_stack(videoTracks, &errorStatus);
                if (!flattened || OTIO_NS::is_error(errorStatus))
                {
                    throw std::runtime_error(ftk::Format(
                        "Cannot flatten the timeline: {0}").
                        arg(errorStatus.full_description));
                }
                track = flattened;
                report.push_back(ftk::Format("Flattened {0} video tracks").
                    arg(videoTracks.size()));
            }
            else if (1 == videoTracks.size())
            {
                track = videoTracks.front();
            }

            size_t transitions = 0;
            size_t gaps = 0;
            size_t markers = 0;
            size_t other = 0;
            if (track)
            {
                for (const auto& child : track->children())
                {
                    if (auto clip = OTIO_NS::dynamic_retainer_cast<OTIO_NS::Clip>(child))
                    {
                        auto item = std::make_shared<FilesModelItem>();
                        item->path = tl::getPath(
                            clip->media_reference(),
                            directory,
                            ftk::PathOptions());
                        if (clip->source_range().has_value())
                        {
                            item->inOutRange = clip->source_range();
                        }

                        item->metadata = clip->metadata();
                        const auto djvC = getMeta(
                            item->metadata, "djv", OTIO_NS::AnyDictionary());
                        // The fields are authoritative once they are read, so
                        // the copy in the metadata would be a second answer.
                        item->metadata.erase("djv");

                        const std::string audioPath = getMeta(
                            djvC, "audioPath", std::string());
                        if (!audioPath.empty())
                        {
                            item->audioPath = tl::getPath(
                                audioPath, directory, ftk::PathOptions());
                        }
                        item->videoLayer = static_cast<size_t>(
                            getMeta(djvC, "videoLayer", int64_t(0)));
                        item->speed = getMetaDouble(djvC, "speed", -1.0);
                        const auto i = djvC.find("currentTime");
                        if (i != djvC.end() &&
                            i->second.type() == typeid(OTIO_NS::RationalTime))
                        {
                            item->currentTime =
                                std::any_cast<OTIO_NS::RationalTime>(i->second);
                        }
                        else if (item->inOutRange.has_value())
                        {
                            // A clip with a range but no saved position
                            // starts at its in point. Starting at zero would
                            // sit before the in point whenever the range
                            // does not begin there, and the player reads
                            // nothing outside the range: a black frame.
                            item->currentTime = item->inOutRange->start_time();
                        }
                        item->framesStated = getMeta(djvC, "framesStated", false);

                        markers += clip->markers().size();
                        out.items.push_back(item);
                    }
                    else if (OTIO_NS::dynamic_retainer_cast<OTIO_NS::Gap>(child))
                    {
                        ++gaps;
                    }
                    else if (OTIO_NS::dynamic_retainer_cast<OTIO_NS::Transition>(child))
                    {
                        ++transitions;
                    }
                    else
                    {
                        ++other;
                    }
                }
                markers += track->markers().size();
            }
            markers += timeline->tracks()->markers().size();

            if (transitions > 0)
            {
                report.push_back(ftk::Format("Dropped {0} transitions").arg(transitions));
            }
            if (gaps > 0)
            {
                report.push_back(ftk::Format("Dropped {0} gaps").arg(gaps));
            }
            if (markers > 0)
            {
                report.push_back(ftk::Format("Dropped {0} markers").arg(markers));
            }
            if (other > 0)
            {
                report.push_back(ftk::Format("Dropped {0} other items").arg(other));
            }
            if (audioTracks > 0)
            {
                report.push_back(ftk::Format("Ignored {0} audio tracks").arg(audioTracks));
            }

            const int size = static_cast<int>(out.items.size());
            out.aIndex = static_cast<int>(getMeta(djvT, "aIndex", int64_t(0)));
            if (out.aIndex < 0 || out.aIndex >= size)
            {
                out.aIndex = size > 0 ? 0 : -1;
            }
            for (const auto& b : getMeta(djvT, "bIndexes", OTIO_NS::AnyVector()))
            {
                if (b.type() == typeid(int64_t))
                {
                    const int index = static_cast<int>(std::any_cast<int64_t>(b));
                    if (index >= 0 && index < size)
                    {
                        out.bIndexes.push_back(index);
                    }
                }
            }
            out.compareOptions = compareFromAny(
                getMeta(djvT, "compareOptions", OTIO_NS::AnyDictionary()));
            tl::CompareTime compareTime = tl::CompareTime::Relative;
            if (from_string(getMeta(djvT, "compareTime", std::string()), compareTime))
            {
                out.compareTime = compareTime;
            }

            return out;
        }

        void playlistSave(
            const std::string& fileName,
            const Playlist& playlist,
            double defaultRate)
        {
            const std::string directory = std::filesystem::u8path(fileName).
                parent_path().u8string();
            auto timeline = playlistToOTIO(playlist, directory, defaultRate);
            OTIO_NS::ErrorStatus errorStatus;
            if (!timeline->to_json_file(fileName, &errorStatus))
            {
                throw std::runtime_error(ftk::Format("Cannot save {0}: {1}").
                    arg(fileName).
                    arg(errorStatus.full_description));
            }
        }

        Playlist playlistOpen(
            const std::string& fileName,
            std::vector<std::string>& report)
        {
            OTIO_NS::ErrorStatus errorStatus;
            OTIO_NS::SerializableObject::Retainer<OTIO_NS::Timeline> timeline(
                dynamic_cast<OTIO_NS::Timeline*>(OTIO_NS::Timeline::from_json_file(
                    fileName, &errorStatus)));
            if (!timeline || OTIO_NS::is_error(errorStatus))
            {
                throw std::runtime_error(ftk::Format("Cannot open {0}: {1}").
                    arg(fileName).
                    arg(errorStatus.full_description));
            }
            const std::string directory = std::filesystem::u8path(fileName).
                parent_path().u8string();
            return playlistFromOTIO(timeline, directory, report);
        }
    }
}
