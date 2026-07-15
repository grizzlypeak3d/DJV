// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <ftk/Core/Range.h>
#include <ftk/Core/Size.h>
#include <ftk/Core/Vector.h>

namespace djv
{
    namespace app
    {
        //! A viewport transform expressed for a specific render size.
        struct RelativeView
        {
            ftk::V2I pos;
            double zoom = 1.0;
        };

        //! Return the zoom that fits a render size inside a viewport.
        double getFitZoom(const ftk::Size2I&, const ftk::Size2I&);

        //! Transfer a view between render and viewport sizes while preserving
        //! its zoom relative to fit and its normalized center.
        bool getRelativeView(
            const ftk::Size2I& previousViewportSize,
            const ftk::Size2I& currentViewportSize,
            const ftk::Size2I& previousRenderSize,
            const ftk::Size2I& currentRenderSize,
            const ftk::V2I& previousPos,
            double previousZoom,
            const ftk::RangeD& zoomRange,
            RelativeView&);
    }
}
