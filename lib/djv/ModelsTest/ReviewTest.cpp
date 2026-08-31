// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsTest/ReviewTest.h>

#include <djv/Models/Review.h>

#include <ftk/Core/Assert.h>
#include <ftk/Core/Format.h>

namespace djv
{
    namespace models_tests
    {
        ReviewTest::ReviewTest(const std::shared_ptr<ftk::Context>& context) :
            ITest(context, "models_tests::ReviewTest")
        {}

        std::shared_ptr<ReviewTest> ReviewTest::create(
            const std::shared_ptr<ftk::Context>& context)
        {
            return std::shared_ptr<ReviewTest>(new ReviewTest(context));
        }

        void ReviewTest::run()
        {
            _version();
            _roundTrip();
            _unreadableSection();
            _unknownSpace();
            _unknownKeys();
        }

        void ReviewTest::_version()
        {
            FTK_CHECK(models::reviewVersionSupported(models::reviewVersion));
            FTK_CHECK(models::reviewVersionSupported(models::reviewVersion - 1));

            // A document from a newer DJV is refused rather than read in part:
            // a best-effort read followed by a save would write the loss back
            // over the author's own file.
            FTK_CHECK(!models::reviewVersionSupported(models::reviewVersion + 1));
        }

        void ReviewTest::_roundTrip()
        {
            models::Review review;

            models::ReviewNote note;
            note.id = "n0";
            note.time = OTIO_NS::RationalTime(24.0, 24.0);
            note.created = "2026-08-26T09:00:00Z";
            note.author = "reviewer";
            note.text = "Too dark here.";
            review.notes.push_back(note);

            models::ReviewStroke stroke;
            stroke.width = 8.F;
            stroke.points.push_back(ftk::V2F(1.F, 2.F));
            stroke.points.push_back(ftk::V2F(3.F, 4.F));

            models::ReviewAnnotation annotation;
            annotation.id = "a0";
            annotation.sourceId = "s0";
            annotation.time = OTIO_NS::RationalTime(48.0, 24.0);
            annotation.author = "reviewer";
            annotation.created = "2026-08-26T09:01:00Z";
            annotation.strokes.push_back(stroke);
            review.annotations.push_back(annotation);

            models::ReviewRange range;
            range.id = "r0";
            range.name = "Opening";
            range.range = OTIO_NS::TimeRange(
                OTIO_NS::RationalTime(0.0, 24.0),
                OTIO_NS::RationalTime(48.0, 24.0));
            review.ranges.push_back(range);

            const nlohmann::json json = review;
            const auto out = json.get<models::Review>();

            FTK_CHECK(out.unreadSections.empty());
            FTK_CHECK(1 == out.notes.size());
            FTK_CHECK(note == out.notes[0]);
            FTK_CHECK(1 == out.annotations.size());
            FTK_CHECK(annotation == out.annotations[0]);
            FTK_CHECK(1 == out.ranges.size());
            FTK_CHECK(range == out.ranges[0]);

            // The author and the time are what make a review readable when it
            // comes back from someone else; they must survive the trip.
            FTK_CHECK("reviewer" == out.notes[0].author);
            FTK_CHECK("reviewer" == out.annotations[0].author);
            FTK_CHECK("2026-08-26T09:01:00Z" == out.annotations[0].created);
        }

        void ReviewTest::_unreadableSection()
        {
            // "color" delegates to serializers that require every key they know,
            // so an incomplete one throws. That must cost the color state and
            // nothing else: the annotations and the notes exist nowhere but here.
            models::Review review;
            models::ReviewNote note;
            note.id = "n0";
            note.created = "2026-08-26T09:00:00Z";
            note.text = "Kept.";
            review.notes.push_back(note);

            nlohmann::json json = review;
            nlohmann::json brokenColor = nlohmann::json::object();
            brokenColor["display"] = nlohmann::json::object();
            json["color"] = brokenColor;

            const auto out = json.get<models::Review>();

            FTK_CHECK(1 == out.unreadSections.size());
            FTK_CHECK("color" == out.unreadSections[0]);
            FTK_CHECK(1 == out.notes.size());
            FTK_CHECK(note == out.notes[0]);

            // Saving leaves the section exactly as it was found. Writing the
            // defaults the application fell back to would destroy state this
            // build merely failed to understand.
            const nlohmann::json saved = out;
            FTK_CHECK(saved.contains("color"));
            FTK_CHECK(brokenColor == saved.at("color"));
        }

        void ReviewTest::_unknownSpace()
        {
            // An annotation in a coordinate space this build does not know
            // cannot be drawn, and drawing it anyway would put it in the wrong
            // place. It is skipped -- and kept, so that an older DJV opening a
            // newer review does not quietly delete someone else's work.
            models::ReviewAnnotation known;
            known.id = "a0";
            known.sourceId = "s0";
            known.strokes.push_back(models::ReviewStroke());

            nlohmann::json foreign = nlohmann::json::object();
            foreign["id"] = "a1";
            foreign["sourceId"] = "s0";
            foreign["space"] = "screen";
            foreign["strokes"] = nlohmann::json::array();

            models::Review review;
            review.annotations.push_back(known);
            nlohmann::json json = review;
            json["annotations"].push_back(foreign);

            const auto out = json.get<models::Review>();

            FTK_CHECK(1 == out.annotations.size());
            FTK_CHECK("a0" == out.annotations[0].id);
            FTK_CHECK(out.unreadSections.empty());

            const nlohmann::json saved = out;
            FTK_CHECK(2 == saved.at("annotations").size());
            FTK_CHECK(foreign == saved.at("annotations")[1]);

            // The same holds for a stroke width in an unknown space: the
            // annotation carrying it is kept whole rather than half-read.
            nlohmann::json foreignWidth = nlohmann::json::object();
            foreignWidth["id"] = "a2";
            foreignWidth["sourceId"] = "s0";
            nlohmann::json strokes = nlohmann::json::array();
            nlohmann::json strokeJson = nlohmann::json::object();
            strokeJson["width"] = 4.F;
            strokeJson["widthSpace"] = "screen";
            strokes.push_back(strokeJson);
            foreignWidth["strokes"] = strokes;

            nlohmann::json json2 = nlohmann::json(models::Review());
            json2["annotations"] = nlohmann::json::array();
            json2["annotations"].push_back(foreignWidth);

            const auto out2 = json2.get<models::Review>();
            FTK_CHECK(out2.annotations.empty());
            const nlohmann::json saved2 = out2;
            FTK_CHECK(1 == saved2.at("annotations").size());
            FTK_CHECK(foreignWidth == saved2.at("annotations")[0]);
        }

        void ReviewTest::_unknownKeys()
        {
            // A section this version knows nothing about survives a load/save
            // cycle untouched, so a review edited by an older DJV keeps whatever
            // a newer one put in it.
            nlohmann::json json = nlohmann::json(models::Review());
            nlohmann::json future = nlohmann::json::object();
            future["something"] = 42;
            json["playlists"] = future;

            const auto out = json.get<models::Review>();
            const nlohmann::json saved = out;

            FTK_CHECK(saved.contains("playlists"));
            FTK_CHECK(future == saved.at("playlists"));
        }
    }
}
