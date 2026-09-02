// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsTest/MarkersModelTest.h>

#include <djv/Models/MarkersModel.h>

#include <ftk/Core/Assert.h>
#include <ftk/Core/Format.h>

namespace djv
{
    namespace models_tests
    {
        MarkersModelTest::MarkersModelTest(const std::shared_ptr<ftk::Context>& context) :
            ITest(context, "models_tests::MarkersModelTest")
        {}

        std::shared_ptr<MarkersModelTest> MarkersModelTest::create(
            const std::shared_ptr<ftk::Context>& context)
        {
            return std::shared_ptr<MarkersModelTest>(new MarkersModelTest(context));
        }

        void MarkersModelTest::run()
        {
            _markers();
            _serialize();
        }

        void MarkersModelTest::_markers()
        {
            auto model = models::MarkersModel::create();

            size_t count = 0;
            auto observer = ftk::ListObserver<models::ReviewMarker>::create(
                model->observeMarkers(),
                [&count](const std::vector<models::ReviewMarker>& value) { count = value.size(); });

            FTK_CHECK(model->getMarkers().empty());

            // Adding fills in the identifier and the creation time; the caller
            // only supplies the rest. Deliberately added out of time order.
            model->add(
                OTIO_NS::TimeRange(
                    OTIO_NS::RationalTime(256.0, 24.0),
                    OTIO_NS::RationalTime(1.0, 24.0)),
                std::string(),
                "Second marker.");
            model->add(
                OTIO_NS::TimeRange(
                    OTIO_NS::RationalTime(128.0, 24.0),
                    OTIO_NS::RationalTime(48.0, 24.0)),
                "Intro",
                std::string());
            model->add(std::nullopt, std::string(), "About the whole review.");
            FTK_CHECK(3 == model->getMarkers().size());
            FTK_CHECK(3 == count);

            // The model keeps time order, with the markers about no frame in
            // particular first.
            const auto& markers = model->getMarkers();
            FTK_CHECK(!markers[0].range.has_value());
            FTK_CHECK(128.0 == markers[1].range->start_time().value());
            FTK_CHECK(256.0 == markers[2].range->start_time().value());
            FTK_CHECK(!markers[0].id.empty());
            FTK_CHECK(!markers[0].created.empty());
            FTK_CHECK(markers[0].id != markers[1].id);
            FTK_CHECK("Intro" == markers[1].name);

            // Updating replaces the text and nothing else: a note added on
            // top of a named span keeps the name.
            const std::string id = markers[1].id;
            model->update(id, "Pacing feels slow here.");
            FTK_CHECK("Pacing feels slow here." == model->getMarkers()[1].text);
            FTK_CHECK("Intro" == model->getMarkers()[1].name);

            // Removing by identifier leaves the other markers alone.
            model->remove(id);
            FTK_CHECK(2 == model->getMarkers().size());

            // Removing an unknown identifier is a no-op.
            model->remove("does-not-exist");
            FTK_CHECK(2 == model->getMarkers().size());

            model->clear();
            FTK_CHECK(model->getMarkers().empty());

            const bool ok =
                0 == model->getMarkers().size() &&
                0 == count;
            _print(ftk::Format("  markers add/update/remove/clear -> {0}").arg(ok ? "ok" : "FAILED"));
            if (!ok)
            {
                _error("MarkersModel add/update/remove/clear failed");
            }
        }

        void MarkersModelTest::_serialize()
        {
            // A marker must survive a round trip through the review file,
            // since that is how it is persisted.
            models::ReviewMarker marker;
            marker.id = "abc123";
            marker.name = "Intro";
            marker.range = OTIO_NS::TimeRange(
                OTIO_NS::RationalTime(218.0, 24.0),
                OTIO_NS::RationalTime(24.0, 24.0));
            marker.color = ftk::Color4F(1.F, 0.F, 0.F, 1.F);
            marker.text = "Line one.\nLine two.";
            marker.author = "reviewer";
            marker.created = "2026-07-26T18:05:00Z";

            const nlohmann::json json = marker;
            const auto out = json.get<models::ReviewMarker>();

            FTK_CHECK(marker == out);

            const bool ok = marker == out;
            _print(ftk::Format("  marker JSON round trip -> {0}").arg(ok ? "ok" : "FAILED"));
            if (!ok)
            {
                _error("ReviewMarker JSON round trip failed");
            }
        }
    }
}
