// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/Models/Parse.h>

#include <tlRender/Timeline/Util.h>

#include <ftk/Core/Format.h>

#include <optional>
#include <stdexcept>

namespace djv
{
    namespace models
    {
        ftk::RangeI64 parseFrameRange(const std::string& value)
        {
            std::optional<ftk::RangeI64> out;
            const size_t i = value.find('-', 1);
            if (i != std::string::npos && i + 1 < value.size())
            {
                const std::string startStr = value.substr(0, i);
                const std::string endStr = value.substr(i + 1);
                try
                {
                    size_t startEnd = 0;
                    size_t endEnd = 0;
                    const int64_t start = std::stoll(startStr, &startEnd);
                    const int64_t end = std::stoll(endStr, &endEnd);
                    // Both halves have to be used up, so that trailing
                    // rubbish is rejected rather than quietly dropped.
                    if (startEnd == startStr.size() && endEnd == endStr.size())
                    {
                        out = ftk::RangeI64(start, end);
                    }
                }
                catch (const std::exception&)
                {}
            }
            if (!out.has_value())
            {
                throw std::runtime_error(
                    ftk::Format("Cannot parse the frame range: \"{0}\", "
                        "expected a range such as \"1-100\"").
                        arg(value));
            }
            return out.value();
        }

        OTIO_NS::RationalTime parseTime(
            const std::string& name,
            const std::string& value,
            double speed,
            tl::TimeUnits units)
        {
            const auto out = tl::textToTime(value, speed, units);
            if (!out.has_value())
            {
                throw std::runtime_error(
                    ftk::Format("Cannot parse the {0}: \"{1}\", expected a "
                        "time in {2}").
                        arg(name).
                        arg(value).
                        arg(tl::getLabel(units)));
            }
            return out.value();
        }
    }
}
