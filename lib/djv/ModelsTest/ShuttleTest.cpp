// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsTest/ShuttleTest.h>

#include <djv/Models/Shuttle.h>

namespace djv
{
    namespace models_tests
    {
        ShuttleTest::ShuttleTest(const std::shared_ptr<ftk::Context>& context) :
            ITest(context, "models_tests::ShuttleTest")
        {}

        std::shared_ptr<ShuttleTest> ShuttleTest::create(
            const std::shared_ptr<ftk::Context>& context)
        {
            return std::shared_ptr<ShuttleTest>(new ShuttleTest(context));
        }

        void ShuttleTest::run()
        {
            // Two seconds at 24 from frame 10: up to 58, exclusive.
            const OTIO_NS::TimeRange range(
                OTIO_NS::RationalTime(10.0, 24.0),
                OTIO_NS::RationalTime(48.0, 24.0));
            const auto frame = [](double value) { return OTIO_NS::RationalTime(value, 24.0); };

            // A frame each 20 pixels, and twice as many at twice the scale.
            FTK_CHECK(frame(20.0) == models::getFrameShuttleTime(frame(20.0), range, 0.F, 1.F));
            FTK_CHECK(frame(22.0) == models::getFrameShuttleTime(frame(20.0), range, 40.F, 1.F));
            FTK_CHECK(frame(18.0) == models::getFrameShuttleTime(frame(20.0), range, -20.F, 2.F));

            // Around from one end to the other.
            FTK_CHECK(frame(56.0) == models::getFrameShuttleTime(frame(10.0), range, -40.F, 1.F));
            FTK_CHECK(frame(13.0) == models::getFrameShuttleTime(frame(56.0), range, 100.F, 1.F));
            FTK_CHECK(frame(10.0) == models::getFrameShuttleTime(frame(57.0), range, 20.F, 1.F));

            // Around more than once.
            FTK_CHECK(frame(24.0) == models::getFrameShuttleTime(frame(20.0), range, 2000.F, 1.F));
            FTK_CHECK(frame(16.0) == models::getFrameShuttleTime(frame(20.0), range, -2000.F, 1.F));
        }
    }
}
