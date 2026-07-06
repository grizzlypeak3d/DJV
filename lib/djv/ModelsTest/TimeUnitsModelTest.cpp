// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsTest/TimeUnitsModelTest.h>

#include <djv/ModelsTest/ModelsTestUtil.h>

#include <djv/Models/TimeUnitsModel.h>

#include <tlRender/Timeline/TimeUnits.h>

#include <ftk/UI/Settings.h>

#include <ftk/Core/Assert.h>
#include <ftk/Core/Observable.h>

#include <filesystem>

namespace djv
{
    namespace models_tests
    {
        TimeUnitsModelTest::TimeUnitsModelTest(const std::shared_ptr<ftk::Context>& context) :
            ITest(context, "models_tests::TimeUnitsModelTest")
        {}

        std::shared_ptr<TimeUnitsModelTest> TimeUnitsModelTest::create(
            const std::shared_ptr<ftk::Context>& context)
        {
            return std::shared_ptr<TimeUnitsModelTest>(new TimeUnitsModelTest(context));
        }

        void TimeUnitsModelTest::run()
        {
            _behavior();
            _persistence();
        }

        void TimeUnitsModelTest::_behavior()
        {
            // In-memory settings keep this test isolated (no file I/O).
            auto settings = createTestSettings(_context);
            auto model = models::TimeUnitsModel::create(_context, settings);

            // Observe the time units. With ObserverAction::Trigger (the default)
            // the callback fires immediately with the current value.
            tl::TimeUnits units = tl::TimeUnits::Frames;
            auto observer = ftk::Observer<tl::TimeUnits>::create(
                model->observeTimeUnits(),
                [&units](const tl::TimeUnits& value) { units = value; });

            // The default (with empty settings) is Timecode.
            FTK_ASSERT(tl::TimeUnits::Timecode == model->getTimeUnits());
            FTK_ASSERT(tl::TimeUnits::Timecode == units);

            // Set the time units; the observer should see each change.
            model->setTimeUnits(tl::TimeUnits::Frames);
            FTK_ASSERT(tl::TimeUnits::Frames == model->getTimeUnits());
            FTK_ASSERT(tl::TimeUnits::Frames == units);

            model->setTimeUnits(tl::TimeUnits::Seconds);
            FTK_ASSERT(tl::TimeUnits::Seconds == model->getTimeUnits());
            FTK_ASSERT(tl::TimeUnits::Seconds == units);
        }

        void TimeUnitsModelTest::_persistence()
        {
            // A real (temporary) file is needed so the state survives across
            // model instances.
            const std::filesystem::path path =
                std::filesystem::temp_directory_path() / "djv-TimeUnitsModelTest.json";
            std::filesystem::remove(path);

            // Change to a non-default value; the model writes it into the settings
            // in its destructor, which then save to disk. Frames (not the Timecode
            // default) proves the value is actually restored rather than defaulted.
            {
                auto settings = ftk::Settings::create(_context, path, true);
                auto model = models::TimeUnitsModel::create(_context, settings);
                model->setTimeUnits(tl::TimeUnits::Frames);
            }

            // Recreate from the same file (reset=false loads it) and verify.
            {
                auto settings = ftk::Settings::create(_context, path, false);
                auto model = models::TimeUnitsModel::create(_context, settings);
                FTK_ASSERT(tl::TimeUnits::Frames == model->getTimeUnits());
            }

            std::filesystem::remove(path);
        }
    }
}
