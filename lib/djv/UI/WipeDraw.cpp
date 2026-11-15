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
            const ftk::Color4F lightColor(1.F, 1.F, 1.F, .9F);
            const ftk::Color4F darkColor(0.F, 0.F, 0.F, .6F);

            //! A ring, its middle left clear.
            void drawRing(
                const std::shared_ptr<ftk::IRender>& render,
                const ftk::V2F& pos,
                float radius,
                float width,
                const ftk::Color4F& color)
            {
                const size_t count = 64;
                const float inner = std::max(0.F, radius - width / 2.F);
                const float outer = radius + width / 2.F;
                ftk::TriMesh2F mesh;
                mesh.v.reserve(count * 2);
                for (size_t i = 0; i < count; ++i)
                {
                    const float a = 2.F * ftk::pi * i / count;
                    const float c = std::cos(a);
                    const float sn = std::sin(a);
                    mesh.v.emplace_back(pos.x + c * outer, pos.y + sn * outer);
                    mesh.v.emplace_back(pos.x + c * inner, pos.y + sn * inner);
                }
                mesh.triangles.reserve(count * 2);
                for (size_t i = 0; i < count; ++i)
                {
                    // One-based, as the meshes take them, and wound as
                    // ftk::circle() winds its triangles: the other way is
                    // culled.
                    const size_t o0 = i * 2 + 1;
                    const size_t i0 = i * 2 + 2;
                    const size_t o1 = ((i + 1) % count) * 2 + 1;
                    const size_t i1 = ((i + 1) % count) * 2 + 2;
                    mesh.triangles.emplace_back(o0, i0, o1);
                    mesh.triangles.emplace_back(i0, i1, o1);
                }
                render->drawMesh(mesh, color);
            }

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
            models::WipeHandle highlight,
            float opacity)
        {
            const float s = displayScale;
            const ftk::Color4F light(lightColor.r, lightColor.g, lightColor.b, lightColor.a * opacity);
            const ftk::Color4F dark(darkColor.r, darkColor.g, darkColor.b, darkColor.a * opacity);

            // The ring that turns it: thin and faint, so it does not hide
            // the seam it crosses, and plain when pointed at or dragged.
            const bool ring = models::WipeHandle::Rotate == highlight;
            const float ringAlpha = ring ? 1.F : .7F;
            drawRing(render, widget.center, widget.radius, (ring ? 5.F : 4.F) * s,
                ftk::Color4F(dark.r, dark.g, dark.b, dark.a * ringAlpha));
            drawRing(render, widget.center, widget.radius, (ring ? 3.F : 2.F) * s,
                ftk::Color4F(light.r, light.g, light.b, light.a * ringAlpha));

            // Ticks where the wipe crosses the ring, for its angle.
            {
                const ftk::V2F d(
                    (widget.rotate.x - widget.center.x) / std::max(widget.radius, 1.F),
                    (widget.rotate.y - widget.center.y) / std::max(widget.radius, 1.F));
                const float t = 6.F * s;
                for (const float side : { 1.F, -1.F })
                {
                    const ftk::V2F p(
                        widget.center.x + d.x * widget.radius * side,
                        widget.center.y + d.y * widget.radius * side);
                    const ftk::V2F a(p.x - d.x * t, p.y - d.y * t);
                    const ftk::V2F b(p.x + d.x * t, p.y + d.y * t);
                    ftk::LineOptions options;
                    options.width = 4.F * s;
                    render->drawLine(a, b, dark, options);
                    options.width = 2.F * s;
                    render->drawLine(a, b, light, options);
                }
            }

            // The dot that moves it.
            const float r = (models::WipeHandle::Center == highlight ? 8.F : 6.F) * s;
            drawDisc(render, widget.center, r + 1.5F * s, dark);
            drawDisc(render, widget.center, r, light);
        }
    }
}
