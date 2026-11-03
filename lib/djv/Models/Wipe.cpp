// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/Models/Wipe.h>

#include <ftk/Core/Math.h>

#include <cmath>

namespace djv
{
    namespace models
    {
        namespace
        {
            float distance(const ftk::V2F& a, const ftk::V2F& b)
            {
                const float dx = a.x - b.x;
                const float dy = a.y - b.y;
                return std::sqrt(dx * dx + dy * dy);
            }
        }

        WipeWidget getWipeWidget(
            const tl::CompareOptions& options,
            const ftk::Box2I& firstBox,
            const ftk::V2F& viewPos,
            float zoom,
            float rotateDistance)
        {
            WipeWidget out;

            // In render space, as the renderer has it: the center a
            // fraction of the first box from its origin, the wipe through
            // it turned by the rotation from upright.
            const float w = static_cast<float>(firstBox.w());
            const float h = static_cast<float>(firstBox.h());
            const ftk::V2F c(w * options.wipeCenter.x, h * options.wipeCenter.y);
            const float rad = ftk::deg2rad(options.wipeRotation + 90.F);
            const ftk::V2F d(std::cos(rad), std::sin(rad));
            const auto toView = [&viewPos, zoom](const ftk::V2F& v)
            {
                return ftk::V2F(viewPos.x + v.x * zoom, viewPos.y + v.y * zoom);
            };
            out.center = toView(c);

            // Up the wipe from the center when it is upright.
            out.rotate = ftk::V2F(
                out.center.x - d.x * rotateDistance,
                out.center.y - d.y * rotateDistance);
            return out;
        }

        WipeHandle getWipeHandle(
            const WipeWidget& widget,
            const ftk::V2F& pos,
            float reach)
        {
            WipeHandle out = WipeHandle::None;
            if (distance(pos, widget.rotate) <= reach)
            {
                out = WipeHandle::Rotate;
            }
            else if (distance(pos, widget.center) <= reach)
            {
                out = WipeHandle::Center;
            }
            return out;
        }

        ftk::V2F getWipeCenter(
            const ftk::V2F& pos,
            const ftk::Box2I& firstBox,
            const ftk::V2F& viewPos,
            float zoom)
        {
            return zoom > 0.F ?
                tl::getWipeCenter(
                    ftk::V2F((pos.x - viewPos.x) / zoom, (pos.y - viewPos.y) / zoom),
                    firstBox) :
                ftk::V2F(.5F, .5F);
        }

        float getWipeRotation(
            const ftk::V2F& center,
            const ftk::V2F& pos)
        {
            // The handle is at the center less the wipe's direction, which
            // for a rotation r is (cos(r + 90), sin(r + 90)): from the center
            // to the handle is (sin r, -cos r).
            const float dx = pos.x - center.x;
            const float dy = pos.y - center.y;
            if (std::abs(dx) < 1.0e-6F && std::abs(dy) < 1.0e-6F)
            {
                return 0.F;
            }
            float out = ftk::rad2deg(std::atan2(dx, -dy));
            if (out < 0.F)
            {
                out += 360.F;
            }
            return out;
        }
    }
}
