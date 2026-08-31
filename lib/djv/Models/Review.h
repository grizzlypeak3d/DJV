// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/Models/Export.h>

#include <djv/Models/SettingsModel.h>
#include <djv/Models/ViewportModel.h>

#include <tlRender/Timeline/ColorOptions.h>
#include <tlRender/Timeline/CompareOptions.h>
#include <tlRender/Timeline/BackgroundOptions.h>
#include <tlRender/Timeline/DisplayOptions.h>
#include <tlRender/Timeline/ForegroundOptions.h>
#include <tlRender/Core/Time.h>

#include <ftk/Core/Vector.h>

#include <nlohmann/json.hpp>

#include <optional>
#include <string>
#include <vector>

namespace djv
{
    namespace models
    {
        //! Current review file format version.
        //!
        //! Incremented only when a document this version writes can no longer be
        //! read correctly by an older DJV. Additive changes -- a new key, a new
        //! section -- keep the version, because the reader ignores what it does
        //! not recognize. A reader that meets a higher version refuses the
        //! document rather than guess at it. See docs/review-format.md.
        constexpr int reviewVersion = 1;

        //! Whether this build can read a review document of the given version.
        //!
        //! False means the document was written by a newer DJV. Reading it
        //! anyway would drop what this build does not understand, and the next
        //! save would write that loss back over the user's file.
        DJV_MODELS_API bool reviewVersionSupported(int version);

        //! \name Optional time comparison
        //! Compare strictly: either both unset, or both set to the same value
        //! at the same rate.
        //!
        //! std::optional would otherwise use OTIO's operator==, which converts
        //! between rates -- frame 12 at 24fps would equal frame 24 at 48fps,
        //! and two annotations on different frames would compare equal.
        ///@{

        DJV_MODELS_API bool sameTime(
            const std::optional<OTIO_NS::RationalTime>&,
            const std::optional<OTIO_NS::RationalTime>&);
        DJV_MODELS_API bool sameRange(
            const std::optional<OTIO_NS::TimeRange>&,
            const std::optional<OTIO_NS::TimeRange>&);

        ///@}

        //! Generate a stable, unique identifier for a review file entry.
        //!
        //! Assigned when a file is opened so that annotations and notes (phase 3)
        //! can reference a file by identity rather than by list index, which is
        //! not stable across close/reorder.
        DJV_MODELS_API std::string generateId();

        //! The current UTC time, ISO 8601 (e.g. "2026-07-26T18:05:00Z").
        DJV_MODELS_API std::string timestamp();

        //! The current user, for attributing notes and annotations.
        //!
        //! Taken from the environment -- USERNAME on Windows, USER elsewhere --
        //! and empty when neither is set. DJV has no accounts and reviews are
        //! passed from hand to hand, so this is a label rather than an identity:
        //! it says who wrote a note when a session comes back from someone else.
        DJV_MODELS_API std::string reviewAuthor();

        //! A single file entry in a review.
        struct DJV_MODELS_API_TYPE ReviewFile
        {
            std::string id;
            std::string path;          //!< Preferred: relative to the .djvr.
            std::string pathAbsolute;  //!< Fallback: absolute path.

            //! Separate audio, resolved like the file above: the relative form
            //! is preferred, the absolute one is the fallback.
            std::string audioPath;
            std::string audioPathAbsolute;
            int         videoLayer = 0;
            double      speed = -1.0;
            std::optional<OTIO_NS::RationalTime> currentTime;
            std::optional<OTIO_NS::TimeRange>    inOutRange;
        };

        //! Comparison setup for a review.
        struct DJV_MODELS_API_TYPE ReviewCompare
        {
            std::string              aId;   //!< The "A" file, by ReviewFile::id.
            std::vector<std::string> bIds;  //!< The "B" files, by ReviewFile::id.
            tl::CompareOptions       options;
            tl::CompareTime          time = tl::CompareTime::Relative;
        };

        //! Viewport view state for a review.
        struct DJV_MODELS_API_TYPE ReviewView
        {
            bool     frameView = true;  //!< If true, pos/zoom are ignored on load.
            ftk::V2I pos;
            double   zoom = 1.0;
        };

        //! Color and image display state for a review.
        struct DJV_MODELS_API_TYPE ReviewColor
        {
            tl::OCIOOptions       ocio;
            tl::LUTOptions        lut;
            tl::DisplayOptions    display;
            tl::BackgroundOptions background;
            tl::ForegroundOptions foreground;
            AspectRatioOptions    aspectRatio;
            HUDOptions            hud;
        };

        //! Interface state for a review. The window layout is
        //! deliberately absent: it belongs to the reader, not the session.
        struct DJV_MODELS_API_TYPE ReviewUI
        {
            //! The open tool panels, in their listed order.
            std::vector<std::string> openTools;
        };

        //! A single freehand stroke of an annotation.
        //!
        //! The points and the width are expressed in the pixels of the source
        //! image, so a stroke keeps its position and its weight whatever the
        //! zoom, the pan or the comparison mode. Both are written with the space
        //! they are expressed in, and a stroke in a space this version does not
        //! know is skipped rather than misplaced. See docs/review-format.md.
        struct DJV_MODELS_API_TYPE ReviewStroke
        {
            ftk::Color4F          color = ftk::Color4F(1.F, .365F, .02F, 1.F);
            float                 width = 4.F;
            std::vector<ftk::V2F> points;

            DJV_MODELS_API bool operator == (const ReviewStroke&) const;
            DJV_MODELS_API bool operator != (const ReviewStroke&) const;
        };

        //! The drawing on one frame of one source.
        struct DJV_MODELS_API_TYPE ReviewAnnotation
        {
            std::string id;

            //! The source the drawing belongs to, by ReviewFile::id.
            std::string sourceId;

            //! The frame the drawing appears on. A drawing is visible on this
            //! frame only.
            std::optional<OTIO_NS::RationalTime> time;

            //! Who drew it, and when, ISO 8601. Both are optional: a document
            //! written before they existed simply leaves them empty.
            std::string author;
            std::string created;

            std::vector<ReviewStroke> strokes;

            DJV_MODELS_API bool operator == (const ReviewAnnotation&) const;
            DJV_MODELS_API bool operator != (const ReviewAnnotation&) const;
        };

        //! A timestamped note attached to a frame of the review.
        //!
        //! Notes belong to the review rather than to a source: the frame is
        //! enough to locate them. See docs/review-format.md.
        struct DJV_MODELS_API_TYPE ReviewNote
        {
            std::string id;

            //! The frame the note refers to, captured when it is published.
            std::optional<OTIO_NS::RationalTime> time;

            //! When the note was published, ISO 8601.
            std::string created;

            //! Who wrote it. Optional: empty when the environment does not say.
            std::string author;

            std::string text;

            DJV_MODELS_API bool operator == (const ReviewNote&) const;
            DJV_MODELS_API bool operator != (const ReviewNote&) const;
        };

        //! A named range of frames in a review.
        //!
        //! Selecting one sets the timeline in/out points. Like notes, a range
        //! belongs to the review rather than to a source: the frames locate it.
        struct DJV_MODELS_API_TYPE ReviewRange
        {
            std::string id;

            //! Free text, defaulted to the frame range when it is created.
            std::string name;

            std::optional<OTIO_NS::TimeRange> range;

            DJV_MODELS_API bool operator == (const ReviewRange&) const;
            DJV_MODELS_API bool operator != (const ReviewRange&) const;
        };

        //! A complete review: files, comparison, view, color and interface state.
        //!
        //! Serialized to a versioned JSON document with the ".djvr" extension.
        struct DJV_MODELS_API_TYPE Review
        {
            int         version = reviewVersion;
            std::string app;
            std::string created;

            std::vector<ReviewFile> files;
            ReviewCompare           compare;
            ReviewView              view;
            ReviewColor             color;
            ReviewUI                ui;
            std::vector<ReviewAnnotation> annotations;
            std::vector<ReviewNote> notes;
            std::vector<ReviewRange> ranges;

            //! The document as loaded, verbatim. Used to carry unknown top-level
            //! sections through a load/save cycle without loss. See
            //! docs/review-format.md.
            nlohmann::json raw;

            //! The top-level sections that were present but could not be read,
            //! by name.
            //!
            //! A section is written by a serializer that requires every key it
            //! knows, so a single key added upstream makes the whole section
            //! fail. Letting that failure through would cost the user the rest
            //! of the document -- the annotations and the notes above all -- so
            //! the section is skipped instead, and saving copies it back out of the
            //! raw document untouched rather than overwriting it with defaults.
            std::vector<std::string> unreadSections;

            //! The items of the annotations, notes, ranges and files lists that
            //! could not be read, verbatim, keyed by section name.
            //!
            //! Those lists are edited during the session, so they cannot simply
            //! be left alone the way a section above is: a new note has to reach
            //! the file. The items that were not understood are re-emitted after
            //! the ones that were, so an annotation drawn by a newer DJV is not
            //! destroyed by an older one opening the review and saving it.
            nlohmann::json unreadItems;
        };

        //! \name Serialize
        ///@{

        DJV_MODELS_API void to_json(nlohmann::json&, const ReviewFile&);
        DJV_MODELS_API void to_json(nlohmann::json&, const ReviewCompare&);
        DJV_MODELS_API void to_json(nlohmann::json&, const ReviewView&);
        DJV_MODELS_API void to_json(nlohmann::json&, const ReviewColor&);
        DJV_MODELS_API void to_json(nlohmann::json&, const ReviewUI&);
        DJV_MODELS_API void to_json(nlohmann::json&, const ReviewStroke&);
        DJV_MODELS_API void to_json(nlohmann::json&, const ReviewAnnotation&);
        DJV_MODELS_API void to_json(nlohmann::json&, const ReviewNote&);
        DJV_MODELS_API void to_json(nlohmann::json&, const ReviewRange&);
        DJV_MODELS_API void to_json(nlohmann::json&, const Review&);

        DJV_MODELS_API void from_json(const nlohmann::json&, ReviewFile&);
        DJV_MODELS_API void from_json(const nlohmann::json&, ReviewCompare&);
        DJV_MODELS_API void from_json(const nlohmann::json&, ReviewView&);
        DJV_MODELS_API void from_json(const nlohmann::json&, ReviewColor&);
        DJV_MODELS_API void from_json(const nlohmann::json&, ReviewUI&);
        DJV_MODELS_API void from_json(const nlohmann::json&, ReviewStroke&);
        DJV_MODELS_API void from_json(const nlohmann::json&, ReviewAnnotation&);
        DJV_MODELS_API void from_json(const nlohmann::json&, ReviewNote&);
        DJV_MODELS_API void from_json(const nlohmann::json&, ReviewRange&);
        DJV_MODELS_API void from_json(const nlohmann::json&, Review&);

        ///@}
    }
}
