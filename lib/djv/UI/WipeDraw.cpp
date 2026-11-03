// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UI/WipeDraw.h>

#include <ftk/UI/DrawUtil.h>

#include <cmath>

namespace djv
{
    namespace ui
    {
        namespace
        {
            const ftk::Color4F light(1.F, 1.F, 1.F, .9F);
            const ftk::Color4F dark(0.F, 0.F, 0.F, .6F);

            void drawDisc(
                const std::shared_ptr<ftk::IRender>& render,
                const ftk::V2F& pos,
                float radius,
                const ftk::Color4F& color)
            {
                render->drawMesh(
                    ftk::circle(
                        ftk::V2I(std::round(pos.x), std::round(pos.y)),
                        std::max(1, static_cast<int>(std::round(radius))),
                        32),
                    color);
            }
        }

        void drawWipe(
            const std::shared_ptr<ftk::IRender>& render,
            const models::WipeWidget& widget,
            float displayScale,
            models::WipeHandle highlight)
        {
            const float s = displayScale;
            // The rotation handle: a ring, on a stalk from the center.
            {
                ftk::LineOptions options;
                options.width = 3.F * s;
                render->drawLine(widget.center, widget.rotate, dark, options);
                options.width = 1.F * s;
                render->drawLine(widget.center, widget.rotate, light, options);
                const float r = (models::WipeHandle::Rotate == highlight ? 9.F : 7.F) * s;
                drawDisc(render, widget.rotate, r + 1.5F * s, dark);
                drawDisc(render, widget.rotate, r, light);
                drawDisc(render, widget.rotate, r - 2.5F * s, dark);
            }

            // The center: a dot.
            const float r = (models::WipeHandle::Center == highlight ? 8.F : 6.F) * s;
            drawDisc(render, widget.center, r + 1.5F * s, dark);
            drawDisc(render, widget.center, r, light);
        }
    }
}
