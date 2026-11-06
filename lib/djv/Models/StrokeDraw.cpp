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
    }
}
