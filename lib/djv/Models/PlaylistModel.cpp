// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/Models/PlaylistModel.h>

#include <tlRender/Core/URL.h>
#include <tlRender/Timeline/Util.h>

#include <ftk/Core/FileIO.h>
#include <ftk/Core/String.h>

#include <opentimelineio/clip.h>
#include <opentimelineio/externalReference.h>
#include <opentimelineio/gap.h>
#include <opentimelineio/track.h>

#include <algorithm>
#include <filesystem>
#include <stdexcept>

namespace djv
{
    namespace models
    {
        namespace
        {
            using RetainedComposable =
                OTIO_NS::SerializableObject::Retainer<OTIO_NS::Composable>;
            using RetainedClip =
                OTIO_NS::SerializableObject::Retainer<OTIO_NS::Clip>;

            bool isOTIOPath(const ftk::Path& path)
            {
                const std::string ext = ftk::toLower(path.getExt());
                return ".otio" == ext || ".otioz" == ext;
            }

            std::string errorText(
                const OTIO_NS::ErrorStatus& errorStatus,
                const std::string& fallback)
            {
                return OTIO_NS::is_error(errorStatus) && !errorStatus.details.empty() ?
                    errorStatus.details : fallback;
            }

            RetainedClip cloneClip(const OTIO_NS::Clip* clip)
            {
                OTIO_NS::ErrorStatus errorStatus;
                auto out = RetainedClip(dynamic_cast<OTIO_NS::Clip*>(
                    clip ? clip->clone(&errorStatus) : nullptr));
                if (!out || OTIO_NS::is_error(errorStatus))
                {
                    throw std::runtime_error(errorText(errorStatus, "Cannot clone OTIO clip."));
                }
                return out;
            }

            void setClipPath(OTIO_NS::Clip* clip, const ftk::Path& path)
            {
                if (!clip)
                {
                    return;
                }
                const std::string absolute = std::filesystem::absolute(
                    std::filesystem::u8path(path.get())).u8string();
                for (const auto& i : clip->media_references())
                {
                    if (auto externalReference =
                        dynamic_cast<OTIO_NS::ExternalReference*>(i.second))
                    {
                        externalReference->set_target_url(tl::encodeURL(absolute));
                    }
                }
                clip->set_name(path.getFileName());
            }

            std::vector<RetainedComposable> retainedChildren(OTIO_NS::Track* track)
            {
                std::vector<RetainedComposable> out;
                if (track)
                {
                    for (const auto& child : track->children())
                    {
                        out.push_back(child);
                    }
                }
                return out;
            }

            bool reorderTrack(
                OTIO_NS::Track* track,
                size_t from,
                size_t to,
                std::string* error)
            {
                auto children = retainedChildren(track);
                if (from >= children.size() || to >= children.size())
                {
                    if (error)
                    {
                        *error = "Playlist reorder index is out of range.";
                    }
                    return false;
                }
                const auto child = children[from];
                OTIO_NS::ErrorStatus errorStatus;
                if (!track->remove_child(static_cast<int>(from), &errorStatus) ||
                    OTIO_NS::is_error(errorStatus))
                {
                    if (error)
                    {
                        *error = errorText(
                            errorStatus,
                            "Cannot remove OTIO playlist item for reordering.");
                    }
                    return false;
                }
                if (!track->insert_child(
                    static_cast<int>(to), child.value, &errorStatus) ||
                    OTIO_NS::is_error(errorStatus))
                {
                    OTIO_NS::ErrorStatus rollbackStatus;
                    track->insert_child(
                        static_cast<int>(std::min(from, track->children().size())),
                        child.value,
                        &rollbackStatus);
                    if (error)
                    {
                        *error = errorText(
                            errorStatus,
                            "Cannot insert reordered OTIO playlist item.");
                    }
                    return false;
                }
                return true;
            }
        }

        struct PlaylistModel::Private
        {
            bool available = false;
            bool editable = false;
            bool dirty = false;
            bool scratch = false;
            ftk::Path path;
            std::string readOnlyReason;
            std::string lastError;
            std::vector<PlaylistItem> items;
            OTIO_NS::SerializableObject::Retainer<OTIO_NS::Timeline> timeline;
            OTIO_NS::Track* videoTrack = nullptr;
            OTIO_NS::Track* audioTrack = nullptr;
            std::shared_ptr<ftk::Observable<int> > revision =
                ftk::Observable<int>::create(0);
        };

        PlaylistModel::PlaylistModel() :
            _p(new Private)
        {}

        PlaylistModel::~PlaylistModel()
        {}

        std::shared_ptr<PlaylistModel> PlaylistModel::create()
        {
            return std::shared_ptr<PlaylistModel>(new PlaylistModel);
        }

        bool PlaylistModel::isAvailable() const { return _p->available; }
        bool PlaylistModel::isEditable() const { return _p->editable; }
        bool PlaylistModel::isDirty() const { return _p->dirty; }
        bool PlaylistModel::isScratch() const { return _p->scratch; }
        const ftk::Path& PlaylistModel::getPath() const { return _p->path; }
        const std::string& PlaylistModel::getReadOnlyReason() const { return _p->readOnlyReason; }
        const std::string& PlaylistModel::getLastError() const { return _p->lastError; }
        const std::vector<PlaylistItem>& PlaylistModel::getItems() const { return _p->items; }
        const OTIO_NS::SerializableObject::Retainer<OTIO_NS::Timeline>& PlaylistModel::getTimeline() const { return _p->timeline; }

        std::shared_ptr<ftk::IObservable<int> > PlaylistModel::observeRevision() const
        {
            return _p->revision;
        }

        void PlaylistModel::clear()
        {
            FTK_P();
            if (!p.available && !p.timeline)
            {
                return;
            }
            p.available = false;
            p.editable = false;
            p.dirty = false;
            p.scratch = false;
            p.path = ftk::Path();
            p.readOnlyReason.clear();
            p.lastError.clear();
            p.items.clear();
            p.timeline = nullptr;
            p.videoTrack = nullptr;
            p.audioTrack = nullptr;
            _changed();
        }

        void PlaylistModel::setTimeline(
            const OTIO_NS::SerializableObject::Retainer<OTIO_NS::Timeline>& timeline,
            const ftk::Path& path,
            bool dirty,
            bool scratch)
        {
            FTK_P();
            if (
                p.timeline.value == timeline.value &&
                p.path.get() == path.get() &&
                p.dirty == dirty &&
                p.scratch == scratch)
            {
                return;
            }
            p.timeline = timeline;
            p.path = path;
            p.dirty = dirty;
            p.scratch = scratch;
            p.lastError.clear();
            _analyze();
            _changed();
        }

        bool PlaylistModel::createFromMedia(
            const std::shared_ptr<ftk::Context>& context,
            const ftk::Path& readPath,
            const ftk::Path& referencePath,
            const ftk::Path& playlistPath,
            const tl::Options& inputOptions,
            std::string* error)
        {
            FTK_P();
            try
            {
                if (isOTIOPath(referencePath))
                {
                    throw std::runtime_error("Select an image or video, not an OTIO timeline.");
                }
                tl::Options options = inputOptions;
                options.pathOptions.seqMaxDigits = 0;
                options.imageSeqAudio = tl::ImageSeqAudio::None;
                auto mediaTimeline =
                    tl::Timeline::create(context, readPath, options);
                const auto& timeline = mediaTimeline->getTimeline();
                for (auto clip : timeline->find_children<OTIO_NS::Clip>())
                {
                    setClipPath(clip.value, referencePath);
                }
                setTimeline(timeline, playlistPath, true, true);
                if (!isEditable())
                {
                    throw std::runtime_error(
                        p.readOnlyReason.empty() ?
                        "The selected media could not be converted to a flat playlist." :
                        p.readOnlyReason);
                }
                if (!_write(playlistPath, error))
                {
                    return false;
                }
                p.dirty = true;
                p.scratch = true;
                p.lastError.clear();
                _changed();
                return true;
            }
            catch (const std::exception& e)
            {
                _setError(e.what());
                if (error)
                {
                    *error = e.what();
                }
                return false;
            }
        }

        bool PlaylistModel::addMedia(
            const std::shared_ptr<ftk::Context>& context,
            const ftk::Path& readPath,
            const ftk::Path& referencePath,
            const tl::Options& inputOptions,
            std::string* error)
        {
            FTK_P();
            try
            {
                if (!p.editable || !p.videoTrack)
                {
                    throw std::runtime_error("The active OTIO timeline is read-only.");
                }
                if (isOTIOPath(referencePath))
                {
                    throw std::runtime_error("Add images or videos, not another OTIO timeline.");
                }

                tl::Options options = inputOptions;
                options.pathOptions.seqMaxDigits = 0;
                options.imageSeqAudio = tl::ImageSeqAudio::None;
                auto mediaTimeline =
                    tl::Timeline::create(context, readPath, options);
                const auto& timeline = mediaTimeline->getTimeline();
                const auto videoTracks = timeline->video_tracks();
                if (videoTracks.size() != 1 || videoTracks[0]->children().empty())
                {
                    throw std::runtime_error("The selected file does not contain supported video or image media.");
                }
                auto sourceVideo = dynamic_cast<OTIO_NS::Clip*>(
                    videoTracks[0]->children()[0].value);
                if (!sourceVideo)
                {
                    throw std::runtime_error("The selected media did not produce an OTIO clip.");
                }
                auto videoClip = cloneClip(sourceVideo);
                setClipPath(videoClip.value, referencePath);
                const OTIO_NS::RationalTime videoDuration = videoClip->duration();

                RetainedClip audioClip;
                const auto audioTracks = timeline->audio_tracks();
                if (!audioTracks.empty() && !audioTracks[0]->children().empty())
                {
                    if (auto sourceAudio = dynamic_cast<OTIO_NS::Clip*>(
                        audioTracks[0]->children()[0].value))
                    {
                        audioClip = cloneClip(sourceAudio);
                        setClipPath(audioClip.value, referencePath);
                    }
                }

                OTIO_NS::ErrorStatus errorStatus;
                const size_t previousVideoCount = p.videoTrack->children().size();
                if (!p.videoTrack->append_child(videoClip.value, &errorStatus) ||
                    OTIO_NS::is_error(errorStatus))
                {
                    throw std::runtime_error(errorText(errorStatus, "Cannot append video clip."));
                }

                if (audioClip && !p.audioTrack)
                {
                    p.audioTrack = new OTIO_NS::Track(
                        "Audio", std::nullopt, OTIO_NS::Track::Kind::audio);
                    for (size_t i = 0; i < previousVideoCount; ++i)
                    {
                        const auto duration = p.videoTrack->children()[i]->duration(&errorStatus);
                        if (!p.audioTrack->append_child(new OTIO_NS::Gap(duration), &errorStatus) ||
                            OTIO_NS::is_error(errorStatus))
                        {
                            throw std::runtime_error(errorText(errorStatus, "Cannot append audio gap."));
                        }
                    }
                    if (!p.timeline->tracks()->append_child(p.audioTrack, &errorStatus) ||
                        OTIO_NS::is_error(errorStatus))
                    {
                        throw std::runtime_error(errorText(errorStatus, "Cannot append audio track."));
                    }
                }
                if (p.audioTrack)
                {
                    OTIO_NS::Composable* audioChild = audioClip ?
                        static_cast<OTIO_NS::Composable*>(audioClip.value) :
                        static_cast<OTIO_NS::Composable*>(new OTIO_NS::Gap(videoDuration));
                    if (!p.audioTrack->append_child(audioChild, &errorStatus) ||
                        OTIO_NS::is_error(errorStatus))
                    {
                        throw std::runtime_error(errorText(errorStatus, "Cannot append audio playlist item."));
                    }
                }

                p.dirty = true;
                p.lastError.clear();
                _analyze();
                _changed();
                return true;
            }
            catch (const std::exception& e)
            {
                _setError(e.what());
                if (error)
                {
                    *error = e.what();
                }
                return false;
            }
        }

        bool PlaylistModel::move(size_t from, size_t to, std::string* error)
        {
            FTK_P();
            if (!p.editable || !p.videoTrack)
            {
                if (error) *error = "The active OTIO timeline is read-only.";
                return false;
            }
            if (from == to)
            {
                return true;
            }
            if (!reorderTrack(p.videoTrack, from, to, error))
            {
                _setError(error ? *error : "Cannot reorder playlist.");
                return false;
            }
            if (p.audioTrack && !reorderTrack(p.audioTrack, from, to, error))
            {
                _setError(error ? *error : "Cannot reorder playlist audio.");
                return false;
            }
            p.dirty = true;
            p.lastError.clear();
            _analyze();
            _changed();
            return true;
        }

        bool PlaylistModel::remove(size_t index, std::string* error)
        {
            FTK_P();
            if (!p.editable || !p.videoTrack)
            {
                if (error) *error = "The active OTIO timeline is read-only.";
                return false;
            }
            if (p.videoTrack->children().size() <= 1)
            {
                if (error) *error = "A playlist must keep at least one media item.";
                return false;
            }
            if (index >= p.videoTrack->children().size())
            {
                if (error) *error = "Playlist remove index is out of range.";
                return false;
            }
            OTIO_NS::ErrorStatus errorStatus;
            if (!p.videoTrack->remove_child(static_cast<int>(index), &errorStatus) ||
                OTIO_NS::is_error(errorStatus))
            {
                const std::string value = errorText(errorStatus, "Cannot remove video playlist item.");
                _setError(value);
                if (error) *error = value;
                return false;
            }
            if (p.audioTrack &&
                (!p.audioTrack->remove_child(static_cast<int>(index), &errorStatus) ||
                OTIO_NS::is_error(errorStatus)))
            {
                const std::string value = errorText(errorStatus, "Cannot remove audio playlist item.");
                _setError(value);
                if (error) *error = value;
                return false;
            }
            p.dirty = true;
            p.lastError.clear();
            _analyze();
            _changed();
            return true;
        }

        bool PlaylistModel::save(const ftk::Path& inputPath, std::string* error)
        {
            FTK_P();
            if (!p.available || !p.timeline)
            {
                if (error) *error = "No OTIO timeline is active.";
                return false;
            }
            ftk::Path path = inputPath;
            if (ftk::toLower(path.getExt()) != ".otio")
            {
                path = ftk::Path(path.get() + ".otio");
            }
            if (!_write(path, error))
            {
                return false;
            }
            p.path = path;
            p.dirty = false;
            p.scratch = false;
            p.lastError.clear();
            _analyze();
            _changed();
            return true;
        }

        bool PlaylistModel::_analyze(std::string* error)
        {
            FTK_P();
            p.available = p.timeline && isOTIOPath(p.path);
            p.editable = false;
            p.videoTrack = nullptr;
            p.audioTrack = nullptr;
            p.items.clear();
            p.readOnlyReason.clear();
            if (!p.available)
            {
                return false;
            }
            if (ftk::toLower(p.path.getExt()) == ".otioz")
            {
                p.readOnlyReason = "Packaged .otioz timelines are read-only. Use Save As to create an .otio copy.";
            }

            const auto videoTracks = p.timeline->video_tracks();
            const auto audioTracks = p.timeline->audio_tracks();
            bool simple = videoTracks.size() == 1 && audioTracks.size() <= 1;
            if (simple)
            {
                const auto& stackChildren = p.timeline->tracks()->children();
                simple = stackChildren.size() == videoTracks.size() + audioTracks.size();
            }
            if (simple)
            {
                p.videoTrack = videoTracks[0];
                p.audioTrack = audioTracks.empty() ? nullptr : audioTracks[0];
                for (const auto& child : p.videoTrack->children())
                {
                    if (!dynamic_cast<OTIO_NS::Clip*>(child.value))
                    {
                        simple = false;
                        break;
                    }
                }
                if (simple && p.audioTrack)
                {
                    simple = p.audioTrack->children().size() == p.videoTrack->children().size();
                    for (const auto& child : p.audioTrack->children())
                    {
                        if (!dynamic_cast<OTIO_NS::Clip*>(child.value) &&
                            !dynamic_cast<OTIO_NS::Gap*>(child.value))
                        {
                            simple = false;
                            break;
                        }
                    }
                }
            }
            if (!simple)
            {
                p.videoTrack = nullptr;
                p.audioTrack = nullptr;
                if (p.readOnlyReason.empty())
                {
                    p.readOnlyReason = "This OTIO contains multiple, nested, transitioned, or unaligned tracks.";
                }
                if (error) *error = p.readOnlyReason;
                return false;
            }

            const std::string directory = std::filesystem::u8path(
                p.path.get()).parent_path().u8string();
            ftk::PathOptions pathOptions;
            pathOptions.seqMaxDigits = 0;
            for (size_t i = 0; i < p.videoTrack->children().size(); ++i)
            {
                auto clip = dynamic_cast<OTIO_NS::Clip*>(
                    p.videoTrack->children()[i].value);
                PlaylistItem item;
                item.path = tl::getPath(clip->media_reference(), directory, pathOptions);
                item.name = clip->name().empty() ? item.path.getFileName() : clip->name();
                item.duration = clip->duration();
                item.hasAudio = p.audioTrack &&
                    dynamic_cast<OTIO_NS::Clip*>(p.audioTrack->children()[i].value);
                std::error_code errorCode;
                item.missing = !item.path.isEmpty() && !std::filesystem::exists(
                    std::filesystem::u8path(item.path.get()), errorCode);
                p.items.push_back(item);
            }
            p.editable = p.readOnlyReason.empty();
            return p.editable;
        }

        bool PlaylistModel::_write(const ftk::Path& path, std::string* error)
        {
            FTK_P();
            try
            {
                OTIO_NS::AnyDictionary dict;
                dict["path"] = path.get();
                dict["audioPath"] = std::string();
                p.timeline->metadata()["tlRender"] = dict;
                OTIO_NS::ErrorStatus errorStatus;
                const std::string json = p.timeline->to_json_string(&errorStatus);
                if (OTIO_NS::is_error(errorStatus))
                {
                    throw std::runtime_error(errorText(errorStatus, "Cannot serialize OTIO timeline."));
                }
                const std::filesystem::path fileName = std::filesystem::u8path(path.get());
                if (!fileName.parent_path().empty())
                {
                    std::filesystem::create_directories(fileName.parent_path());
                }
                auto fileIO = ftk::FileIO::create(fileName, ftk::FileMode::Write);
                fileIO->write(json);
                return true;
            }
            catch (const std::exception& e)
            {
                _setError(e.what());
                if (error) *error = e.what();
                return false;
            }
        }

        void PlaylistModel::_setError(const std::string& value)
        {
            _p->lastError = value;
            _changed();
        }

        void PlaylistModel::_changed()
        {
            _p->revision->setAlways(_p->revision->get() + 1);
        }
    }
}
