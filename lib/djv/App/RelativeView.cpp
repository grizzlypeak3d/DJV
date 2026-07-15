// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/RelativeView.h>

#include <algorithm>
#include <cmath>

namespace djv
{
    namespace app
    {
        namespace
        {
            bool isValid(const ftk::Size2I& value)
            {
                return value.w > 0 && value.h > 0;
            }
        }

        double getFitZoom(
            const ftk::Size2I& viewportSize,
            const ftk::Size2I& renderSize)
        {
            if (!isValid(viewportSize) || !isValid(renderSize))
            {
                return 0.0;
            }
            return std::min(
                viewportSize.w / static_cast<double>(renderSize.w),
                viewportSize.h / static_cast<double>(renderSize.h));
        }

        bool getRelativeView(
            const ftk::Size2I& previousViewportSize,
            const ftk::Size2I& currentViewportSize,
            const ftk::Size2I& previousRenderSize,
            const ftk::Size2I& currentRenderSize,
            const ftk::V2I& previousPos,
            double previousZoom,
            const ftk::RangeD& zoomRange,
            RelativeView& out)
        {
            const double previousFitZoom = getFitZoom(
                previousViewportSize,
                previousRenderSize);
            const double currentFitZoom = getFitZoom(
                currentViewportSize,
                currentRenderSize);
            if (previousFitZoom <= 0.0 || currentFitZoom <= 0.0 ||
                previousZoom <= 0.0 || !std::isfinite(previousZoom))
            {
                return false;
            }

            const double relativeZoom = previousZoom / previousFitZoom;
            const double currentZoom = std::clamp(
                currentFitZoom * relativeZoom,
                zoomRange.min(),
                zoomRange.max());
            if (currentZoom <= 0.0 || !std::isfinite(currentZoom))
            {
                return false;
            }

            const double previousViewportCenterX = previousViewportSize.w / 2.0;
            const double previousViewportCenterY = previousViewportSize.h / 2.0;
            const double normalizedCenterX =
                (previousViewportCenterX - previousPos.x) /
                previousZoom /
                previousRenderSize.w;
            const double normalizedCenterY =
                (previousViewportCenterY - previousPos.y) /
                previousZoom /
                previousRenderSize.h;

            const double currentViewportCenterX = currentViewportSize.w / 2.0;
            const double currentViewportCenterY = currentViewportSize.h / 2.0;
            out.pos = ftk::V2I(
                static_cast<int>(std::lround(
                    currentViewportCenterX -
                    normalizedCenterX * currentRenderSize.w * currentZoom)),
                static_cast<int>(std::lround(
                    currentViewportCenterY -
                    normalizedCenterY * currentRenderSize.h * currentZoom)));
            out.zoom = currentZoom;
            return true;
        }
    }
}
