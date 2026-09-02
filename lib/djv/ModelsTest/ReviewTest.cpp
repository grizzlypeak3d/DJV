// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsTest/ReviewTest.h>

#include <djv/Models/Review.h>

#include <opentimelineio/clip.h>
#include <opentimelineio/externalReference.h>
#include <opentimelineio/marker.h>
#include <opentimelineio/stack.h>
#include <opentimelineio/track.h>

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
            _courtesyRead();
            _otioMarkers();
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

            models::ReviewMarker marker;
            marker.id = "m0";
            marker.range = OTIO_NS::TimeRange(
                OTIO_NS::RationalTime(24.0, 24.0),
                OTIO_NS::RationalTime(1.0, 24.0));
            marker.created = "2026-08-26T09:00:00Z";
            marker.author = "reviewer";
            marker.text = "Too dark here.";
            review.markers.push_back(marker);

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

            models::ReviewMarker span;
            span.id = "m1";
            span.name = "Opening";
            span.range = OTIO_NS::TimeRange(
                OTIO_NS::RationalTime(0.0, 24.0),
                OTIO_NS::RationalTime(48.0, 24.0));
            review.markers.push_back(span);

            const nlohmann::json json = review;
            const auto out = json.get<models::Review>();

            FTK_CHECK(out.unreadSections.empty());
            FTK_CHECK(2 == out.markers.size());
            FTK_CHECK(marker == out.markers[0]);
            FTK_CHECK(span == out.markers[1]);
            FTK_CHECK(1 == out.annotations.size());
            FTK_CHECK(annotation == out.annotations[0]);

            // The author and the time are what make a review readable when it
            // comes back from someone else; they must survive the trip.
            FTK_CHECK("reviewer" == out.markers[0].author);
            FTK_CHECK("reviewer" == out.annotations[0].author);
            FTK_CHECK("2026-08-26T09:01:00Z" == out.annotations[0].created);
        }

        void ReviewTest::_unreadableSection()
        {
            // "color" delegates to serializers that require every key they know,
            // so an incomplete one throws. That must cost the color state and
            // nothing else: the annotations and the markers exist nowhere but here.
            models::Review review;
            models::ReviewMarker marker;
            marker.id = "m0";
            marker.created = "2026-08-26T09:00:00Z";
            marker.text = "Kept.";
            review.markers.push_back(marker);

            nlohmann::json json = review;
            nlohmann::json brokenColor = nlohmann::json::object();
            brokenColor["display"] = nlohmann::json::object();
            json["color"] = brokenColor;

            const auto out = json.get<models::Review>();

            FTK_CHECK(1 == out.unreadSections.size());
            FTK_CHECK("color" == out.unreadSections[0]);
            FTK_CHECK(1 == out.markers.size());
            FTK_CHECK(marker == out.markers[0]);

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

        void ReviewTest::_courtesyRead()
        {
            // The development-era "notes" and "ranges" sections lift into
            // markers on load: a note becomes a one-frame marker with its
            // text, a range a marker with its name and span. Format version 1
            // never shipped with them, so this path lasts one development
            // cycle.
            nlohmann::json json = nlohmann::json(models::Review());
            nlohmann::json note = nlohmann::json::object();
            note["id"] = "n0";
            note["time"] = { { "value", 24.0 }, { "rate", 24.0 } };
            note["created"] = "2026-08-26T09:00:00Z";
            note["author"] = "reviewer";
            note["text"] = "Too dark here.";
            json["notes"] = nlohmann::json::array({ note });
            nlohmann::json range = nlohmann::json::object();
            range["id"] = "r0";
            range["name"] = "Opening";
            range["range"] = {
                { "start", { { "value", 0.0 }, { "rate", 24.0 } } },
                { "duration", { { "value", 48.0 }, { "rate", 24.0 } } } };
            json["ranges"] = nlohmann::json::array({ range });

            const auto out = json.get<models::Review>();
            FTK_CHECK(2 == out.markers.size());
            FTK_CHECK("n0" == out.markers[0].id);
            FTK_CHECK("Too dark here." == out.markers[0].text);
            FTK_CHECK(out.markers[0].range.has_value());
            FTK_CHECK(24.0 == out.markers[0].range->start_time().value());
            FTK_CHECK(1.0 == out.markers[0].range->duration().value());
            FTK_CHECK("reviewer" == out.markers[0].author);
            FTK_CHECK("r0" == out.markers[1].id);
            FTK_CHECK("Opening" == out.markers[1].name);
            FTK_CHECK(48.0 == out.markers[1].range->duration().value());

            // Saving writes markers and drops the lifted sections; left in
            // place they would come back as duplicates on the next read.
            const nlohmann::json saved = out;
            FTK_CHECK(saved.contains("markers"));
            FTK_CHECK(2 == saved.at("markers").size());
            FTK_CHECK(!saved.contains("notes"));
            FTK_CHECK(!saved.contains("ranges"));
        }

        void ReviewTest::_otioMarkers()
        {
            // The round trip: markers written to a timeline's stack come
            // back whole -- identity, attribution, text as the comment,
            // and a rangeless marker restored by its flag.
            OTIO_NS::ErrorStatus errorStatus;
            OTIO_NS::SerializableObject::Retainer<OTIO_NS::Timeline> timeline(
                new OTIO_NS::Timeline);
            auto track = new OTIO_NS::Track(
                "Video", std::nullopt, OTIO_NS::Track::Kind::video);
            auto clip = new OTIO_NS::Clip(
                "clip",
                new OTIO_NS::ExternalReference("media.mov"),
                OTIO_NS::TimeRange(
                    OTIO_NS::RationalTime(0.0, 24.0),
                    OTIO_NS::RationalTime(100.0, 24.0)));
            track->append_child(clip, &errorStatus);
            timeline->tracks()->append_child(track, &errorStatus);
            FTK_CHECK(!OTIO_NS::is_error(errorStatus));

            std::vector<models::ReviewMarker> markers;
            models::ReviewMarker span;
            span.id = "m0";
            span.name = "Intro";
            span.range = OTIO_NS::TimeRange(
                OTIO_NS::RationalTime(10.0, 24.0),
                OTIO_NS::RationalTime(20.0, 24.0));
            span.color = ftk::Color4F(1.F, 0.F, 0.F, 1.F);
            span.text = "Too slow.";
            span.author = "reviewer";
            span.created = "2026-09-02T10:00:00Z";
            markers.push_back(span);
            models::ReviewMarker rangeless;
            rangeless.id = "m1";
            rangeless.text = "About the whole review.";
            markers.push_back(rangeless);

            models::reviewMarkersToTimeline(markers, timeline);
            const auto stackMarkers = timeline->tracks()->markers();
            FTK_CHECK(2 == stackMarkers.size());
            FTK_CHECK("Too slow." == stackMarkers[0]->comment());

            const auto out = models::reviewMarkersFromTimeline(timeline);
            FTK_CHECK(2 == out.size());
            FTK_CHECK(span == out[0]);
            FTK_CHECK(rangeless == out[1]);

            // A foreign marker on a clip: its time transforms to the
            // timeline's, its comment becomes the text, and it gets a
            // fresh identity with no false attribution.
            OTIO_NS::AnyDictionary metadata;
            clip->markers().push_back(
                OTIO_NS::SerializableObject::Retainer<OTIO_NS::Marker>(
                    new OTIO_NS::Marker(
                        "editor note",
                        OTIO_NS::TimeRange(
                            OTIO_NS::RationalTime(5.0, 24.0),
                            OTIO_NS::RationalTime(1.0, 24.0)),
                        OTIO_NS::Color::red,
                        metadata,
                        "From the editor.")));
            const auto out2 = models::reviewMarkersFromTimeline(timeline);
            FTK_CHECK(3 == out2.size());
            FTK_CHECK("editor note" == out2[2].name);
            FTK_CHECK("From the editor." == out2[2].text);
            FTK_CHECK(out2[2].range.has_value());
            FTK_CHECK(5.0 == out2[2].range->start_time().value());
            FTK_CHECK(!out2[2].id.empty());
            FTK_CHECK(out2[2].author.empty());
        }
    }
}
