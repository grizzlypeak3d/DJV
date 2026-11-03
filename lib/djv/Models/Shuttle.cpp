// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/Models/Shuttle.h>

#include <cmath>

namespace djv
{
    namespace models
    {
        OTIO_NS::RationalTime getFrameShuttleTime(
            const OTIO_NS::RationalTime& start,
            const OTIO_NS::TimeRange& timeRange,
            float dx,
            float scale)
        {
            const OTIO_NS::RationalTime offset = OTIO_NS::RationalTime(
                dx * .05F * scale,
                start.rate()).round();
            // Around as many times as it takes: a long drag over a short
            // range goes past both ends.
            const double rate = start.rate();
            const double duration = timeRange.duration().rescaled_to(rate).value();
            if (duration <= 0.0)
            {
                return timeRange.start_time();
            }
            double value = (start + offset - timeRange.start_time()).rescaled_to(rate).value();
            value = std::fmod(value, duration);
            if (value < 0.0)
            {
                value += duration;
            }
            return timeRange.start_time().rescaled_to(rate) + OTIO_NS::RationalTime(value, rate);
        }
    }
}
