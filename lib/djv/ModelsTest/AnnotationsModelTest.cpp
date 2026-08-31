// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsTest/AnnotationsModelTest.h>

#include <djv/Models/AnnotationsModel.h>

#include <ftk/Core/Assert.h>
#include <ftk/Core/Format.h>

namespace djv
{
    namespace models_tests
    {
        namespace
        {
            const OTIO_NS::RationalTime frame(const double value)
            {
                return OTIO_NS::RationalTime(value, 24.0);
            }

            models::ReviewStroke makeStroke(float x0, float y0, float x1, float y1)
            {
                models::ReviewStroke out;
                out.width = 4.F;
                out.points.push_back(ftk::V2F(x0, y0));
                out.points.push_back(ftk::V2F(x1, y1));
                return out;
            }
        }

        AnnotationsModelTest::AnnotationsModelTest(const std::shared_ptr<ftk::Context>& context) :
            ITest(context, "models_tests::AnnotationsModelTest")
        {}

        std::shared_ptr<AnnotationsModelTest> AnnotationsModelTest::create(
            const std::shared_ptr<ftk::Context>& context)
        {
            return std::shared_ptr<AnnotationsModelTest>(new AnnotationsModelTest(context));
        }

        void AnnotationsModelTest::run()
        {
            _strokes();
            _erase();
            _undo();
            _serialize();
        }

        void AnnotationsModelTest::_strokes()
        {
            auto model = models::AnnotationsModel::create();
            FTK_CHECK(model->getAnnotations().empty());

            // Strokes on the same source and frame join one annotation.
            model->addStroke("srcA", frame(100), makeStroke(0.F, 0.F, 10.F, 10.F));
            model->addStroke("srcA", frame(100), makeStroke(20.F, 20.F, 30.F, 30.F));
            FTK_CHECK(1 == model->getAnnotations().size());
            FTK_CHECK(2 == model->getStrokes("srcA", frame(100)).size());

            // A different frame, or a different source, is a separate annotation:
            // a drawing is visible on its own frame only.
            model->addStroke("srcA", frame(200), makeStroke(0.F, 0.F, 5.F, 5.F));
            model->addStroke("srcB", frame(100), makeStroke(0.F, 0.F, 5.F, 5.F));
            FTK_CHECK(3 == model->getAnnotations().size());
            FTK_CHECK(model->getStrokes("srcA", frame(999)).empty());

            const bool ok =
                3 == model->getAnnotations().size() &&
                2 == model->getStrokes("srcA", frame(100)).size() &&
                1 == model->getStrokes("srcB", frame(100)).size() &&
                model->getStrokes("srcA", frame(999)).empty();
            _print(ftk::Format("  strokes grouped per source and frame -> {0}").arg(ok ? "ok" : "FAILED"));
            if (!ok)
            {
                _error("Annotation stroke grouping failed");
            }
        }

        void AnnotationsModelTest::_erase()
        {
            auto model = models::AnnotationsModel::create();
            model->addStroke("srcA", frame(100), makeStroke(0.F, 0.F, 10.F, 0.F));
            model->addStroke("srcA", frame(100), makeStroke(0.F, 100.F, 10.F, 100.F));

            // The eraser deletes whole strokes it touches, and leaves the others.
            model->eraseStrokes("srcA", frame(100), ftk::V2F(5.F, 0.F), 2.F);
            const bool erasedOne = 1 == model->getStrokes("srcA", frame(100)).size();

            // Far from any stroke, nothing is erased.
            model->eraseStrokes("srcA", frame(100), ftk::V2F(500.F, 500.F), 2.F);
            const bool untouched = 1 == model->getStrokes("srcA", frame(100)).size();

            // Erasing the last stroke drops the annotation, so no phantom marker
            // is left behind in the timeline.
            model->eraseStrokes("srcA", frame(100), ftk::V2F(5.F, 100.F), 2.F);
            const bool dropped = model->getAnnotations().empty();

            FTK_CHECK(erasedOne);
            FTK_CHECK(untouched);
            FTK_CHECK(dropped);
            const bool ok = erasedOne && untouched && dropped;
            _print(ftk::Format("  eraser removes touched strokes only -> {0}").arg(ok ? "ok" : "FAILED"));
            if (!ok)
            {
                _error("Annotation erase failed");
            }
        }

        void AnnotationsModelTest::_undo()
        {
            auto model = models::AnnotationsModel::create();

            bool hasUndo = false;
            auto undoObserver = ftk::Observer<bool>::create(
                model->observeHasUndo(),
                [&hasUndo](bool value) { hasUndo = value; });

            FTK_CHECK(!hasUndo);
            model->addStroke("srcA", frame(100), makeStroke(0.F, 0.F, 10.F, 10.F));
            model->addStroke("srcA", frame(100), makeStroke(20.F, 20.F, 30.F, 30.F));
            FTK_CHECK(hasUndo);
            FTK_CHECK(2 == model->getStrokes("srcA", frame(100)).size());

            // Undo is multi-level: each stroke is its own step.
            model->undo();
            const bool oneLeft = 1 == model->getStrokes("srcA", frame(100)).size();
            model->undo();
            const bool noneLeft = model->getAnnotations().empty();

            // And redo brings them back.
            model->redo();
            const bool redone = 1 == model->getStrokes("srcA", frame(100)).size();

            FTK_CHECK(oneLeft);
            FTK_CHECK(noneLeft);
            FTK_CHECK(redone);
            const bool ok = oneLeft && noneLeft && redone;
            _print(ftk::Format("  multi-level undo and redo -> {0}").arg(ok ? "ok" : "FAILED"));
            if (!ok)
            {
                _error("Annotation undo/redo failed");
            }
        }

        void AnnotationsModelTest::_serialize()
        {
            // Annotations must survive the review file, points included.
            models::ReviewAnnotation annotation;
            annotation.id = "abc123";
            annotation.sourceId = "src456";
            annotation.time = frame(218);
            annotation.strokes.push_back(makeStroke(812.5F, 430.F, 815.F, 433.5F));

            const nlohmann::json json = annotation;
            const auto out = json.get<models::ReviewAnnotation>();

            FTK_CHECK(annotation == out);
            const bool ok = annotation == out;
            _print(ftk::Format("  annotation JSON round trip -> {0}").arg(ok ? "ok" : "FAILED"));
            if (!ok)
            {
                _error("ReviewAnnotation JSON round trip failed");
            }
        }
    }
}
