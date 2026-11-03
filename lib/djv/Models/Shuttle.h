// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/Models/Export.h>

#include <opentimelineio/version.h>
#include <opentime/timeRange.h>

namespace djv
{
    namespace models
    {
        //! \name Frame Shuttle
        //! Dragging sideways over the picture to step through the frames,
        //! with the mouse on the desktop and a finger on the iPad.
        ///@{

        //! The time the frame shuttle is at, a drag of so many of the view's
        //! pixels from where it began: a frame each 20 pixels at a scale of
        //! one, around from one end of the time range to the other, and
        //! never past its end.
        DJV_MODELS_API OTIO_NS::RationalTime getFrameShuttleTime(
            const OTIO_NS::RationalTime& start,
            const OTIO_NS::TimeRange&,
            float dx,
            float scale);

        ///@}
    }
}
