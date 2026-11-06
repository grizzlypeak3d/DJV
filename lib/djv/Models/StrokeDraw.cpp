// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/Models/StrokeDraw.h>

#include <djv/Models/Stroke.h>

#include <cmath>

#include <algorithm>

namespace djv
{
    namespace models
    {
        void drawStroke(
            const std::shared_ptr<ftk::IRender>& render,
            const std::shared_ptr<ftk::FontSystem>& fontSystem,
            const models::ReviewStroke& stroke,
            const std::function<ftk::V2F(const ftk::V2F&)>& toScreen,
            float scale,
            float alpha,
            bool caret)
        {
            if (stroke.points.empty())
            {
                return;
            }
            ftk::Color4F color = stroke.color;
            color.a *= alpha;

            std::vector<ftk::V2F> path;
            path.reserve(stroke.points.size());
            for (const auto& point : stroke.points)
            {
                path.push_back(toScreen(point));
            }
            const float width = std::max(1.F, stroke.width * scale);

            switch (stroke.kind)
            {
            case models::ReviewStrokeKind::Freehand:
            {
                // Thin the captured points before smoothing, then subdivide:
                // clustered controls are what make the spline overshoot.
                // A pen's pressure goes along, and sets the width at each
                // point: from a third of the width at the lightest touch
                // to a third over it at the hardest.
                std::vector<ftk::V2F> simplified;
                std::vector<float> simplifiedPressures;
                models::simplifyPath(path, stroke.pressures, 3.F, simplified, simplifiedPressures);
                std::vector<ftk::V2F> smoothed;
                std::vector<float> smoothedPressures;
                models::smoothPath(simplified, simplifiedPressures, 12, smoothed, smoothedPressures);
                if (!smoothed.empty() && smoothedPressures.size() == smoothed.size())
                {
                    std::vector<float> widths;
                    widths.reserve(smoothed.size());
                    for (float pressure : smoothedPressures)
                    {
                        widths.push_back(width * std::clamp(.35F + pressure, .35F, 1.35F));
                    }
                    render->drawMesh(models::strokeMesh(smoothed, widths), color);
                }
                else
                {
                    render->drawMesh(models::strokeMesh(smoothed, width), color);
                }
                break;
            }
            case models::ReviewStrokeKind::Text:
            {
                // The text is laid out at the size it has where it is drawn,
                // so it is as sharp as the interface's own.
                ftk::FontInfo fontInfo;
                fontInfo.size = std::max(1, static_cast<int>(std::round(stroke.textSize * scale)));
                const auto fontMetrics = fontSystem->getMetrics(fontInfo);
                const auto glyphs = fontSystem->getGlyphs(stroke.text, fontInfo);
                render->drawText(glyphs, fontMetrics, path.front(), color);
                if (caret)
                {
                    // A bar where the next character goes.
                    const int textW = stroke.text.empty() ? 0 : fontSystem->getSize(stroke.text, fontInfo).w;
                    render->drawRect(
                        ftk::Box2F(
                            path.front().x + textW,
                            path.front().y,
                            std::max(1.F, fontInfo.size / 12.F),
                            static_cast<float>(fontMetrics.lineHeight)),
                        color);
                }
                break;
            }
            default:
                render->drawMesh(models::shapeMesh(stroke.kind, path, width), color);
                break;
            }
        }
    
        void drawSelection(
            const std::shared_ptr<ftk::IRender>& render,
            const std::shared_ptr<ftk::FontSystem>& fontSystem,
            const models::ReviewStroke& stroke,
            const std::function<ftk::V2F(const ftk::V2F&)>& toScreen,
            float scale,
            float displayScale)
        {
            if (stroke.points.empty())
            {
                return;
            }
            // What the stroke covers on the screen.
            float minX = 0.F, minY = 0.F, maxX = 0.F, maxY = 0.F;
            bool first = true;
            auto add = [&](const ftk::V2F& point)
            {
                const ftk::V2F s = toScreen(point);
                if (first)
                {
                    minX = maxX = s.x;
                    minY = maxY = s.y;
                    first = false;
                }
                minX = std::min(minX, s.x);
                minY = std::min(minY, s.y);
                maxX = std::max(maxX, s.x);
                maxY = std::max(maxY, s.y);
            };
            const float s = std::max(.001F, scale);
            if (models::ReviewStrokeKind::Text == stroke.kind)
            {
                ftk::FontInfo fontInfo;
                fontInfo.size = std::max(1, static_cast<int>(std::round(stroke.textSize * s)));
                const ftk::Size2I size = fontSystem->getSize(stroke.text, fontInfo);
                const ftk::V2F& a = stroke.points.front();
                const float w = size.w / s;
                const float h = std::max(static_cast<float>(size.h), fontInfo.size * 1.F) / s;
                add(a);
                add(ftk::V2F(a.x + w, a.y + h));
            }
            else
            {
                for (const auto& point : models::shapePath(stroke.kind, stroke.points))
                {
                    add(point);
                }
            }
            // The two points of a shape, as handles to drag.
            for (const auto& point : models::strokeHandles(stroke))
            {
                const ftk::V2F p = toScreen(point);
                const float h = selectionHandleSize * displayScale;
                render->drawRect(
                    ftk::Box2F(p.x - h / 2.F - 1.F, p.y - h / 2.F - 1.F, h + 2.F, h + 2.F),
                    ftk::Color4F(0.F, 0.F, 0.F, .6F));
                render->drawRect(
                    ftk::Box2F(p.x - h / 2.F, p.y - h / 2.F, h, h),
                    ftk::Color4F(1.F, 1.F, 1.F, .95F));
            }
            const float pad = stroke.width * s / 2.F + 6.F;
            const ftk::Box2F box(minX - pad, minY - pad, maxX - minX + pad * 2.F, maxY - minY + pad * 2.F);
            const ftk::Color4F color(1.F, 1.F, 1.F, .85F);
            const ftk::Color4F shadow(0.F, 0.F, 0.F, .5F);
            for (int pass = 0; pass < 2; ++pass)
            {
                const float t = pass ? 1.F : 3.F;
                const float o = pass ? 0.F : 1.F;
                const ftk::Color4F& c = pass ? color : shadow;
                render->drawRect(ftk::Box2F(box.min.x - o, box.min.y - o, box.w() + o * 2.F, t), c);
                render->drawRect(ftk::Box2F(box.min.x - o, box.max.y - t + o + 1.F, box.w() + o * 2.F, t), c);
                render->drawRect(ftk::Box2F(box.min.x - o, box.min.y - o, t, box.h() + o * 2.F), c);
                render->drawRect(ftk::Box2F(box.max.x - t + o + 1.F, box.min.y - o, t, box.h() + o * 2.F), c);
            }
        }
    }
}
