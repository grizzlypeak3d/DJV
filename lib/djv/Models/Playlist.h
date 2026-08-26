// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/Models/FilesModel.h>

#include <opentimelineio/timeline.h>

namespace djv
{
    namespace models
    {
        //! A playlist: the open file list and the compare state, as saved to
        //! and opened from an ".otio" file.
        //!
        //! A playlist is deliberately not a session. What belongs here is
        //! what stops making sense when the set of open files changes -- the
        //! order, the per-file state, and the A/B comparison. Color, view,
        //! and window state stay in the settings.
        struct DJV_MODELS_API_TYPE Playlist
        {
            std::vector<std::shared_ptr<FilesModelItem> > items;

            int              aIndex = -1;
            std::vector<int> bIndexes;

            tl::CompareOptions compareOptions;
            tl::CompareTime    compareTime = tl::CompareTime::Relative;
        };

        //! Create an OTIO timeline from a playlist: a single video track with
        //! one clip per item. DJV's per-item state goes in each clip's
        //! metadata under a "djv" key, and the compare state in the
        //! timeline's metadata, so another application sees a plain timeline
        //! and DJV's state rides along without getting in its way.
        //!
        //! References are written relative to the directory when the media
        //! can be reached from it, so a playlist saved beside its media moves
        //! with it; everything else is written absolute.
        //!
        //! The rate is for a sequence that has not been opened, which has no
        //! rate of its own to write.
        DJV_MODELS_API OTIO_NS::SerializableObject::Retainer<OTIO_NS::Timeline> playlistToOTIO(
            const Playlist&,
            const std::string& directory,
            double defaultRate);

        //! Create a playlist from an OTIO timeline. Relative references are
        //! resolved against the directory.
        //!
        //! Any timeline is accepted, not only ones DJV wrote: multiple video
        //! tracks are flattened, and whatever the file list cannot carry --
        //! transitions, effects, markers, audio tracks -- is dropped and
        //! reported, one line per kind. A timeline of only audio opens its
        //! first audio track, since the file list holds audio files the same
        //! as anything else. The source file is never rewritten, so the
        //! flattening loses nothing on disk.
        //!
        //! Clip metadata is kept on each item, so what another application
        //! wrote survives a save from DJV.
        DJV_MODELS_API Playlist playlistFromOTIO(
            const OTIO_NS::SerializableObject::Retainer<OTIO_NS::Timeline>&,
            const std::string& directory,
            std::vector<std::string>& report);

        //! Save a playlist to an ".otio" file.
        DJV_MODELS_API void playlistSave(
            const std::string& fileName,
            const Playlist&,
            double defaultRate);

        //! Open a playlist from an ".otio" file.
        //!
        //! Throws on a file that cannot be read, and on a playlist written by
        //! a newer DJV, since silently dropping what the newer version wrote
        //! would corrupt it on the next save.
        DJV_MODELS_API Playlist playlistOpen(
            const std::string& fileName,
            std::vector<std::string>& report);
    }
}
