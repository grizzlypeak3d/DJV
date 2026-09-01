// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/Models/Export.h>

#include <tlRender/Timeline/TimeUnits.h>

#include <ftk/Core/Range.h>

#include <opentime/rationalTime.h>

#include <string>

namespace djv
{
    namespace models
    {
        //! Parse a frame range such as "1-100", and "-10-20" for a sequence
        //! starting before zero: the separator is the first dash after the
        //! first character, so a negative start is not mistaken for it.
        //! Throws on anything else.
        DJV_MODELS_API ftk::RangeI64 parseFrameRange(const std::string&);

        //! Parse a time in the given units. The name is for the error
        //! message ("in point", "seek time"), which is thrown.
        DJV_MODELS_API OTIO_NS::RationalTime parseTime(
            const std::string& name,
            const std::string& value,
            double speed,
            tl::TimeUnits);
    }
}
