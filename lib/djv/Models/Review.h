// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

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
        //! Current review file format version. See docs/ROADMAP_REVIEW_SESSIONS.md.
        constexpr int reviewVersion = 1;

        //! \name Optional time comparison
        //! Compare strictly: either both unset, or both set to the same value
        //! at the same rate.
        //!
        //! std::optional would otherwise use OTIO's operator==, which converts
        //! between rates -- frame 12 at 24fps would equal frame 24 at 48fps,
        //! and two annotations on different frames would compare equal.
        ///@{

        bool sameTime(
            const std::optional<OTIO_NS::RationalTime>&,
            const std::optional<OTIO_NS::RationalTime>&);
        bool sameRange(
            const std::optional<OTIO_NS::TimeRange>&,
            const std::optional<OTIO_NS::TimeRange>&);

        ///@}

        //! Generate a stable, unique identifier for a review file entry.
        //!
        //! Assigned when a file is opened so that annotations and notes (phase 3)
        //! can reference a file by identity rather than by list index, which is
        //! not stable across close/reorder.
        std::string generateId();

        //! The current UTC time, ISO 8601 (e.g. "2026-07-26T18:05:00Z").
        std::string timestamp();

        //! A single file entry in a review.
        struct ReviewFile
        {
            std::string id;
            std::string path;          //!< Preferred: relative to the .djvr.
            std::string pathAbsolute;  //!< Fallback: absolute path.
            std::string audioPath;     //!< Separate audio, absolute.
            int         videoLayer = 0;
            double      speed = -1.0;
            std::optional<OTIO_NS::RationalTime> currentTime;
            std::optional<OTIO_NS::TimeRange>    inOutRange;
        };

        //! Comparison setup for a review.
        struct ReviewCompare
        {
            std::string              aId;   //!< The "A" file, by ReviewFile::id.
            std::vector<std::string> bIds;  //!< The "B" files, by ReviewFile::id.
            tl::CompareOptions       options;
            tl::CompareTime          time = tl::CompareTime::Relative;
        };

        //! Viewport view state for a review.
        struct ReviewView
        {
            bool     frameView = true;  //!< If true, pos/zoom are ignored on load.
            ftk::V2I pos;
            double   zoom = 1.0;
        };

        //! Color and image display state for a review.
        struct ReviewColor
        {
            tl::OCIOOptions       ocio;
            tl::LUTOptions        lut;
            tl::DisplayOptions    display;
            tl::BackgroundOptions background;
            tl::ForegroundOptions foreground;
            AspectRatioOptions    aspectRatio;
            HUDOptions            hud;
        };

        //! Interface state for a review.
        struct ReviewUI
        {
            std::string    activeTool;
            WindowSettings window;
        };

        //! A single freehand stroke of an annotation.
        //!
        //! The points and the width are expressed in the pixels of the source
        //! image, so a stroke keeps its position and its weight whatever the
        //! zoom, the pan or the comparison mode. See
        //! docs/ROADMAP_REVIEW_SESSIONS.md section 7.1.
        struct ReviewStroke
        {
            ftk::Color4F          color = ftk::Color4F(1.F, .365F, .02F, 1.F);
            float                 width = 4.F;
            std::vector<ftk::V2F> points;

            bool operator == (const ReviewStroke&) const;
            bool operator != (const ReviewStroke&) const;
        };

        //! The drawing on one frame of one source.
        struct ReviewAnnotation
        {
            std::string id;

            //! The source the drawing belongs to, by ReviewFile::id.
            std::string sourceId;

            //! The frame the drawing appears on. A drawing is visible on this
            //! frame only.
            std::optional<OTIO_NS::RationalTime> time;

            std::vector<ReviewStroke> strokes;

            bool operator == (const ReviewAnnotation&) const;
            bool operator != (const ReviewAnnotation&) const;
        };

        //! A timestamped note attached to a frame of the review.
        //!
        //! Notes belong to the review rather than to a source: the frame is
        //! enough to locate them. See docs/ROADMAP_REVIEW_SESSIONS.md section
        //! 7.2.bis.
        struct ReviewNote
        {
            std::string id;

            //! The frame the note refers to, captured when it is published.
            std::optional<OTIO_NS::RationalTime> time;

            //! When the note was published, ISO 8601.
            std::string created;

            std::string text;

            bool operator == (const ReviewNote&) const;
            bool operator != (const ReviewNote&) const;
        };

        //! A named range of frames in a review.
        //!
        //! Selecting one sets the timeline in/out points. Like notes, a range
        //! belongs to the review rather than to a source: the frames locate it.
        struct ReviewRange
        {
            std::string id;

            //! Free text, defaulted to the frame range when it is created.
            std::string name;

            std::optional<OTIO_NS::TimeRange> range;

            bool operator == (const ReviewRange&) const;
            bool operator != (const ReviewRange&) const;
        };

        //! A complete review: files, comparison, view, color and interface state.
        //!
        //! Serialized to a versioned JSON document with the ".djvr" extension.
        struct Review
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
            //! sections (e.g. future "annotations"/"notes") through a load/save
            //! cycle without loss. See docs/ROADMAP_REVIEW_SESSIONS.md §4.4.
            nlohmann::json raw;
        };

        //! \name Serialize
        ///@{

        void to_json(nlohmann::json&, const ReviewFile&);
        void to_json(nlohmann::json&, const ReviewCompare&);
        void to_json(nlohmann::json&, const ReviewView&);
        void to_json(nlohmann::json&, const ReviewColor&);
        void to_json(nlohmann::json&, const ReviewUI&);
        void to_json(nlohmann::json&, const ReviewStroke&);
        void to_json(nlohmann::json&, const ReviewAnnotation&);
        void to_json(nlohmann::json&, const ReviewNote&);
        void to_json(nlohmann::json&, const ReviewRange&);
        void to_json(nlohmann::json&, const Review&);

        void from_json(const nlohmann::json&, ReviewFile&);
        void from_json(const nlohmann::json&, ReviewCompare&);
        void from_json(const nlohmann::json&, ReviewView&);
        void from_json(const nlohmann::json&, ReviewColor&);
        void from_json(const nlohmann::json&, ReviewUI&);
        void from_json(const nlohmann::json&, ReviewStroke&);
        void from_json(const nlohmann::json&, ReviewAnnotation&);
        void from_json(const nlohmann::json&, ReviewNote&);
        void from_json(const nlohmann::json&, ReviewRange&);
        void from_json(const nlohmann::json&, Review&);

        ///@}
    }
}
