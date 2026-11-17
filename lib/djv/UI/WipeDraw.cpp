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
            const ftk::Color4F diskColor(0.F, 0.F, 0.F, .35F);
            const ftk::Color4F edgeColor(1.F, 1.F, 1.F, .35F);
            const ftk::Color4F outlineColor(0.F, 0.F, 0.F, .6F);

            //! How much lighter a part is pointed at and pressed: the dark
            //! style's hover and pressed overlays.
            const float hoverLight = .1F;
            const float pressedLight = .2F;

            //! The dot, from its resting gray toward white as it is pointed
            //! at and pressed. A white overlay does not show on a light dot,
            //! so its steps are larger.
            const float dotGray = .7F;
            const float dotHoverGray = .85F;
            const float dotPressedGray = 1.F;

            ftk::Color4F faded(const ftk::Color4F& c, float opacity)
            {
                return ftk::Color4F(c.r, c.g, c.b, c.a * opacity);
            }

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
                const ftk::V2I& pos,
                float radius,
                const ftk::Color4F& color)
            {
                render->drawMesh(
                    ftk::circle(
                        pos,
                        std::max(1, static_cast<int>(std::round(radius))),
                        64),
                    color);
            }
        }

        void drawWipe(
            const std::shared_ptr<ftk::IRender>& render,
            const std::shared_ptr<ftk::FontSystem>& fontSystem,
            const ftk::FontInfo& fontInfo,
            const models::WipeWidget& widget,
            float displayScale,
            models::WipeHandle hover,
            models::WipeHandle pressed,
            float opacity)
        {
            const float s = displayScale;
            // On whole pixels, so the dot sits in the middle of the disk.
            const ftk::V2I c(std::round(widget.center.x), std::round(widget.center.y));
            const ftk::V2F cf(c.x, c.y);

            // The band that turns it, around the middle that moves it,
            // which is left clear so the seam shows through.
            const float inner = widget.radius * models::wipeMoveFraction;
            const float bandRadius = (inner + widget.radius) / 2.F;
            const float bandWidth = widget.radius - inner;
            drawRing(render, cf, bandRadius, bandWidth, faded(diskColor, opacity));
            const float light =
                models::WipeHandle::Rotate == pressed ? pressedLight :
                models::WipeHandle::Rotate == hover ? hoverLight :
                0.F;
            if (light > 0.F)
            {
                drawRing(render, cf, bandRadius, bandWidth, ftk::Color4F(1.F, 1.F, 1.F, light * opacity));
            }
            drawRing(render, cf, widget.radius, 1.F * s, faded(edgeColor, opacity));
            drawRing(render, cf, inner, 1.F * s, faded(edgeColor, opacity));

            // Two ticks across the band where the seam crosses it, which
            // show the wipe's angle when the seam itself does not: A and B
            // the same, or nearly.
            {
                const float base = std::atan2(widget.rotate.y - cf.y, widget.rotate.x - cf.x);
                const float r0 = inner + 2.F * s;
                const float r1 = widget.radius - 2.F * s;
                // A pixel and a half wide: one pixel lying on the pixel
                // edges can drop out at some angles.
                const float hw = .75F * s;
                ftk::TriMesh2F mesh;
                for (int i = 0; i < 2; ++i)
                {
                    const float a = base + i * ftk::pi;
                    const ftk::V2F d(std::cos(a), std::sin(a));
                    const ftk::V2F n(-d.y * hw, d.x * hw);
                    const size_t b = mesh.v.size();
                    mesh.v.emplace_back(cf.x + d.x * r0 + n.x, cf.y + d.y * r0 + n.y);
                    mesh.v.emplace_back(cf.x + d.x * r1 + n.x, cf.y + d.y * r1 + n.y);
                    mesh.v.emplace_back(cf.x + d.x * r1 - n.x, cf.y + d.y * r1 - n.y);
                    mesh.v.emplace_back(cf.x + d.x * r0 - n.x, cf.y + d.y * r0 - n.y);
                    // One-based, as the meshes take them.
                    mesh.triangles.emplace_back(b + 1, b + 2, b + 3);
                    mesh.triangles.emplace_back(b + 1, b + 3, b + 4);
                }
                render->drawMesh(mesh, faded(edgeColor, opacity));
            }

            // The middle, lit faintly while it is pointed at or pressed,
            // so how much of it moves the wipe shows.
            const float middleLight =
                models::WipeHandle::Center == pressed ? pressedLight :
                models::WipeHandle::Center == hover ? hoverLight :
                0.F;
            if (middleLight > 0.F)
            {
                drawDisc(render, c, inner, ftk::Color4F(1.F, 1.F, 1.F, middleLight * opacity));
            }

            // The dot that moves it.
            const float gray =
                models::WipeHandle::Center == pressed ? dotPressedGray :
                models::WipeHandle::Center == hover ? dotHoverGray :
                dotGray;
            const float r = 4.F * s;
            drawDisc(render, c, r + 1.5F * s, faded(outlineColor, opacity));
            drawDisc(render, c, r, ftk::Color4F(gray, gray, gray, opacity));

            // "A" and "B" on the band, each on its own side of the seam,
            // a quarter turn from the ticks: A, which the wipe shows on
            // the left of the seam when it is upright. The band is their
            // background; a dark outline keeps them clear of a light
            // picture, which shows through it.
            if (fontSystem)
            {
                const float ux = widget.rotate.x - cf.x;
                const float uy = widget.rotate.y - cf.y;
                const float ul = std::sqrt(ux * ux + uy * uy);
                if (ul > 0.F)
                {
                    const ftk::V2F side(uy / ul, -ux / ul);
                    // Sized to the band, which is narrower than a line of
                    // the interface's text.
                    ftk::FontInfo font = fontInfo;
                    font.size = std::max(1, static_cast<int>(std::round(bandWidth * .85F)));
                    const ftk::FontMetrics metrics = fontSystem->getMetrics(font);
                    const float outline = std::max(1.F, std::round(s));
                    const ftk::Color4F textColor(1.F, 1.F, 1.F, .9F * opacity);
                    const ftk::Color4F textOutline(0.F, 0.F, 0.F, .6F * opacity);
                    for (const auto& [letter, sign] : {
                        std::make_pair(std::string("A"), 1.F),
                        std::make_pair(std::string("B"), -1.F) })
                    {
                        const ftk::Size2I size = fontSystem->getSize(letter, font);
                        const ftk::V2F pos(
                            std::round(cf.x + side.x * sign * bandRadius - size.w / 2.F),
                            std::round(cf.y + side.y * sign * bandRadius - size.h / 2.F));
                        const auto glyphs = fontSystem->getGlyphs(letter, font);
                        for (const auto& o : {
                            ftk::V2F(-outline, 0.F), ftk::V2F(outline, 0.F),
                            ftk::V2F(0.F, -outline), ftk::V2F(0.F, outline) })
                        {
                            render->drawText(glyphs, metrics, ftk::V2F(pos.x + o.x, pos.y + o.y), textOutline);
                        }
                        render->drawText(glyphs, metrics, pos, textColor);
                    }
                }
            }
        }
    }
}
