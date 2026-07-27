// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <tlRender/Timeline/Timeline.h>

#include <ftk/Core/Observable.h>
#include <ftk/Core/Path.h>

namespace djv
{
    namespace models
    {
        //! An item in an editable OTIO media playlist.
        struct PlaylistItem
        {
            std::string name;
            ftk::Path path;
            OTIO_NS::RationalTime duration;
            bool hasAudio = false;
            bool missing = false;
        };

        //! Model for a flat OTIO media playlist.
        //!
        //! Editable playlists contain one video track of clips and an optional
        //! aligned audio track containing clips or gaps. Other OTIO structures
        //! remain available for inspection and Save As, but are read-only.
        class PlaylistModel : public std::enable_shared_from_this<PlaylistModel>
        {
            FTK_NON_COPYABLE(PlaylistModel);

        protected:
            PlaylistModel();

        public:
            ~PlaylistModel();

            static std::shared_ptr<PlaylistModel> create();

            bool isAvailable() const;
            bool isEditable() const;
            bool isDirty() const;
            bool isScratch() const;
            const ftk::Path& getPath() const;
            const std::string& getReadOnlyReason() const;
            const std::string& getLastError() const;
            const std::vector<PlaylistItem>& getItems() const;
            const OTIO_NS::SerializableObject::Retainer<OTIO_NS::Timeline>& getTimeline() const;

            std::shared_ptr<ftk::IObservable<int> > observeRevision() const;

            void clear();
            void setTimeline(
                const OTIO_NS::SerializableObject::Retainer<OTIO_NS::Timeline>&,
                const ftk::Path&,
                bool dirty = false,
                bool scratch = false);

            bool createFromMedia(
                const std::shared_ptr<ftk::Context>&,
                const ftk::Path& readPath,
                const ftk::Path& referencePath,
                const ftk::Path& playlistPath,
                const tl::Options&,
                std::string* error = nullptr);

            bool addMedia(
                const std::shared_ptr<ftk::Context>&,
                const ftk::Path& readPath,
                const ftk::Path& referencePath,
                const tl::Options&,
                std::string* error = nullptr);

            bool move(size_t from, size_t to, std::string* error = nullptr);
            bool remove(size_t index, std::string* error = nullptr);
            bool save(const ftk::Path&, std::string* error = nullptr);

        private:
            bool _analyze(std::string* error = nullptr);
            bool _write(const ftk::Path&, std::string* error = nullptr);
            void _setError(const std::string&);
            void _changed();

            FTK_PRIVATE();
        };
    }
}
