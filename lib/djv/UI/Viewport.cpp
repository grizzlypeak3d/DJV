// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UI/Viewport.h>

#include <djv/Models/AnnotationsModel.h>
#include <djv/Models/ColorModel.h>
#include <djv/Models/DrawModel.h>
#include <djv/Models/FilesModel.h>
#include <djv/Models/SettingsModel.h>
#include <djv/Models/TimeUnitsModel.h>
#include <djv/Models/ViewportModel.h>

#include <tlRender/Timeline/Util.h>

#include <ftk/UI/ColorSwatch.h>
#include <ftk/UI/IWindow.h>
#include <ftk/UI/Label.h>
#include <ftk/UI/SysLogModel.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/ScreenshotTag.h>
#include <ftk/UI/Spacer.h>
#include <ftk/Core/Format.h>
#include <ftk/Core/String.h>
#include <ftk/Core/Timer.h>

#include <algorithm>

#include <cmath>
#include <regex>

namespace
{
    //! Drop points closer together than the given distance.
    //!
    //! Freehand input arrives in dense clusters, and clustered control points
    //! make a Catmull-Rom spline overshoot; thinning first is what keeps the
    //! curve clean.
    std::vector<ftk::V2F> simplifyPath(const std::vector<ftk::V2F>& points, float minDistance)
    {
        std::vector<ftk::V2F> out;
        if (points.empty())
        {
            return out;
        }
        out.push_back(points.front());
        const float minSquared = minDistance * minDistance;
        for (size_t i = 1; i + 1 < points.size(); ++i)
        {
            const float dx = points[i].x - out.back().x;
            const float dy = points[i].y - out.back().y;
            if (dx * dx + dy * dy >= minSquared)
            {
                out.push_back(points[i]);
            }
        }
        if (points.size() > 1)
        {
            out.push_back(points.back());
        }
        return out;
    }

    //! Smooth a path with a centripetal Catmull-Rom spline.
    //!
    //! The curve passes through every control point while rounding the corners.
    //! The centripetal parameterisation (alpha = 0.5) is what avoids the cusps
    //! and self-intersections that the uniform form produces on unevenly spaced
    //! points.
    std::vector<ftk::V2F> smoothPath(const std::vector<ftk::V2F>& points, int subdivisions)
    {
        if (points.size() < 3)
        {
            return points;
        }
        auto knot = [](float t, const ftk::V2F& a, const ftk::V2F& b)
        {
            const float dx = b.x - a.x;
            const float dy = b.y - a.y;
            // alpha = 0.5 -> the fourth root of the squared distance.
            return t + std::pow(std::sqrt(dx * dx + dy * dy), .5F);
        };
        std::vector<ftk::V2F> out;
        out.reserve(points.size() * subdivisions);
        const size_t size = points.size();
        for (size_t i = 0; i + 1 < size; ++i)
        {
            // Duplicate the ends so the first and last spans are drawn too.
            const ftk::V2F& p0 = points[i > 0 ? i - 1 : 0];
            const ftk::V2F& p1 = points[i];
            const ftk::V2F& p2 = points[i + 1];
            const ftk::V2F& p3 = points[i + 2 < size ? i + 2 : size - 1];

            const float t0 = 0.F;
            const float t1 = knot(t0, p0, p1);
            const float t2 = knot(t1, p1, p2);
            const float t3 = knot(t2, p2, p3);
            if (t2 - t1 <= 0.F)
            {
                continue;
            }
            for (int j = 0; j < subdivisions; ++j)
            {
                const float t = t1 + (t2 - t1) * (j / static_cast<float>(subdivisions));
                // Barry-Goldman pyramidal evaluation, guarding the spans that
                // collapse when two control points coincide.
                auto lerp = [](const ftk::V2F& a, const ftk::V2F& b, float ta, float tb, float t)
                {
                    const float d = tb - ta;
                    if (d <= 0.F)
                    {
                        return a;
                    }
                    const float u = (t - ta) / d;
                    return ftk::V2F(
                        a.x + (b.x - a.x) * u,
                        a.y + (b.y - a.y) * u);
                };
                const ftk::V2F a1 = lerp(p0, p1, t0, t1, t);
                const ftk::V2F a2 = lerp(p1, p2, t1, t2, t);
                const ftk::V2F a3 = lerp(p2, p3, t2, t3, t);
                const ftk::V2F b1 = lerp(a1, a2, t0, t2, t);
                const ftk::V2F b2 = lerp(a2, a3, t1, t3, t);
                out.push_back(lerp(b1, b2, t1, t2, t));
            }
        }
        out.push_back(points.back());
        return out;
    }

    //! Append a filled disc to a mesh, used for the two end caps.
    //!
    //! The number of sides follows the radius: a fixed count would show flat
    //! facets once a stroke is thick or zoomed in. Mesh vertex indices are
    //! one-based.
    void addDisc(ftk::TriMesh2F& mesh, const ftk::V2F& center, float radius)
    {
        const int segments = std::max(16, std::min(96, static_cast<int>(radius * 2.F)));
        const size_t centerIndex = mesh.v.size() + 1;
        mesh.v.push_back(center);
        for (int i = 0; i < segments; ++i)
        {
            const float a = i / static_cast<float>(segments) * 2.F * 3.14159265F;
            mesh.v.push_back(ftk::V2F(
                center.x + std::cos(a) * radius,
                center.y + std::sin(a) * radius));
        }
        for (int i = 0; i < segments; ++i)
        {
            // Wind the same way as the ribbon quads: the renderer culls back
            // faces, so a disc wound the other way is simply never drawn.
            ftk::Triangle2 triangle;
            triangle.v[0].v = centerIndex;
            triangle.v[1].v = centerIndex + 1 + ((i + 1) % segments);
            triangle.v[2].v = centerIndex + 1 + i;
            mesh.triangles.push_back(triangle);
        }
    }

    //! Build a mesh for a stroke as one continuous ribbon.
    //!
    //! Consecutive quads share their vertices, so the outline has no gap and no
    //! overlap anywhere along the path -- that is what removes the notches that
    //! independent per-segment quads leave on the outside of a curve. Each point
    //! is offset along the averaged normal of its two neighbouring segments (a
    //! mitre), lengthened by 1/cos to hold the width through a turn and clamped
    //! so a sharp corner cannot spike. Round caps close the two ends.
    ftk::TriMesh2F strokeMesh(const std::vector<ftk::V2F>& points, float width)
    {
        ftk::TriMesh2F out;
        const float radius = std::max(.5F, width / 2.F);
        if (points.empty())
        {
            return out;
        }
        if (1 == points.size())
        {
            addDisc(out, points[0], radius);
            return out;
        }

        auto direction = [](const ftk::V2F& a, const ftk::V2F& b)
        {
            const float dx = b.x - a.x;
            const float dy = b.y - a.y;
            const float length = std::sqrt(dx * dx + dy * dy);
            return length > 0.F ?
                ftk::V2F(dx / length, dy / length) :
                ftk::V2F(0.F, 0.F);
        };

        const size_t size = points.size();
        for (size_t i = 0; i < size; ++i)
        {
            const ftk::V2F dirPrev = i > 0 ?
                direction(points[i - 1], points[i]) :
                direction(points[0], points[1]);
            const ftk::V2F dirNext = i + 1 < size ?
                direction(points[i], points[i + 1]) :
                direction(points[size - 2], points[size - 1]);

            ftk::V2F tangent(dirPrev.x + dirNext.x, dirPrev.y + dirNext.y);
            const float length = std::sqrt(tangent.x * tangent.x + tangent.y * tangent.y);
            if (length > 0.F)
            {
                tangent.x /= length;
                tangent.y /= length;
            }
            else
            {
                tangent = dirNext;
            }

            float offset = radius;
            const float cosHalf = dirNext.x * tangent.x + dirNext.y * tangent.y;
            if (cosHalf > .25F)
            {
                offset = std::min(radius / cosHalf, radius * 3.F);
            }
            const ftk::V2F normal(-tangent.y * offset, tangent.x * offset);

            out.v.push_back(ftk::V2F(points[i].x + normal.x, points[i].y + normal.y));
            out.v.push_back(ftk::V2F(points[i].x - normal.x, points[i].y - normal.y));
        }

        for (size_t i = 0; i + 1 < size; ++i)
        {
            // One-based indices: left(i) = 2i+1, right(i) = 2i+2.
            const size_t left = i * 2 + 1;
            const size_t right = left + 1;
            const size_t leftNext = left + 2;
            const size_t rightNext = right + 2;
            ftk::Triangle2 triangle;
            triangle.v[0].v = left;
            triangle.v[1].v = leftNext;
            triangle.v[2].v = rightNext;
            out.triangles.push_back(triangle);
            triangle.v[0].v = left;
            triangle.v[1].v = rightNext;
            triangle.v[2].v = right;
            out.triangles.push_back(triangle);
        }

        addDisc(out, points.front(), radius);
        addDisc(out, points.back(), radius);
        return out;
    }
}

namespace djv
{
    namespace ui
    {
        namespace
        {
            // Long enough for a short path, short enough that the message does
            // not run the width of the viewport. The untruncated text is in
            // the messages tool.
            const size_t toastTextLength = 80;

            const std::chrono::seconds toastTimeout(5);
        }

        struct Viewport::Private
        {
            std::weak_ptr<models::ViewportModel> viewportModel;
            std::weak_ptr<models::TimeUnitsModel> timeUnitsModel;
            models::HUDOptions hudOptions;
            ftk::Path path;
            tl::IOInfo ioInfo;
            std::optional<OTIO_NS::RationalTime> currentTime;
            double fps = 0.0;
            size_t droppedFrames = 0;
            size_t videoFramesSize = 0;
            std::vector<std::string> ocioInputs;
            // Whether the picture stands in for a frame the media does not
            // have, and the frame it repeats when there is one.
            bool missing = false;
            std::optional<int64_t> heldFrom;
            // Kept for the annotation hit test and overlay, which need the
            // image sizes to map screen to image pixels.
            std::vector<tl::VideoFrame> videoFrames;
            ftk::ImageOptions imageOptions;
            tl::DisplayOptions displayOptions;
            tl::PlayerCacheInfo cacheInfo;
            double viewZoom = 0.0;
            models::MouseActionBinding frameShuttleBinding =
                models::MouseActionBinding(ftk::MouseButton::Left, ftk::KeyModifier::Shift);
            float frameShuttleScale = 1.F;

            std::shared_ptr<ftk::Label> fileNameLabel;
            std::shared_ptr<ftk::Label> cacheLabel;
            std::shared_ptr<ftk::Label> timeLabel;
            std::shared_ptr<ftk::Label> viewZoomLabel;
            std::shared_ptr<ftk::ColorSwatch> colorPickerSwatch;
            std::shared_ptr<ftk::Label> colorPickerLabel;
            std::shared_ptr<ftk::Label> infoLabel;
            std::shared_ptr<ftk::Label> renderLabel;
            std::map<models::HUDItem, std::shared_ptr<ftk::IWidget> > hudWidgets;
            tl::Compare compare = tl::Compare::None;
            tl::CompareTime compareTime = tl::CompareTime::Relative;
            std::shared_ptr<models::FilesModelItem> a;
            std::vector<std::shared_ptr<models::FilesModelItem> > b;
            std::shared_ptr<ftk::Label> compareLabel;

            bool toastActive = false;
            bool hudActive = true;
            std::shared_ptr<ftk::Label> toastLabel;
            std::shared_ptr<ftk::Timer> toastTimer;
            std::shared_ptr<ftk::VerticalLayout> hudLayout;
            std::map<models::HUDPos, std::shared_ptr<ftk::VerticalLayout> > hudLayouts;

            std::shared_ptr<ftk::Observer<OTIO_NS::RationalTime> > currentTimeObserver;
            std::shared_ptr<ftk::Observer<std::string> > mediaReferenceKeyObserver;
            std::shared_ptr<ftk::ListObserver<tl::VideoFrame> > videoObserver;
            std::shared_ptr<ftk::Observer<tl::PlayerCacheInfo> > cacheObserver;
            std::shared_ptr<ftk::Observer<double> > fpsObserver;
            std::shared_ptr<ftk::Observer<size_t> > droppedFramesObserver;
            std::shared_ptr<ftk::Observer<std::shared_ptr<models::FilesModelItem> > > aObserver;
            std::shared_ptr<ftk::ListObserver<std::shared_ptr<models::FilesModelItem> > > bObserver;
            std::shared_ptr<ftk::Observer<tl::CompareOptions> > compareOptionsObserver;
            std::shared_ptr<ftk::Observer<tl::CompareTime> > compareTimeObserver;
            std::shared_ptr<ftk::Observer<bool> > drawEnabledObserver;
            std::shared_ptr<ftk::Observer<tl::OCIOOptions> > ocioOptionsObserver;
            std::shared_ptr<ftk::Observer<std::vector<std::string> > > resolvedInputsObserver;
            std::shared_ptr<ftk::Observer<tl::LUTOptions> > lutOptionsObserver;
            std::shared_ptr<ftk::Observer<ftk::ImageOptions> > imageOptionsObserver;
            std::shared_ptr<ftk::Observer<tl::DisplayOptions> > displayOptionsObserver;
            std::shared_ptr<ftk::Observer<tl::BackgroundOptions> > bgOptionsObserver;
            std::shared_ptr<ftk::Observer<tl::ForegroundOptions> > fgOptionsObserver;
            std::shared_ptr<ftk::Observer<ftk::gl::TextureType> > colorBufferObserver;
            std::shared_ptr<ftk::Observer<double> > viewZoomObserver;
            std::shared_ptr<ftk::ListObserver<ftk::LogItem> > messagesObserver;
            std::shared_ptr<ftk::Observer<models::HUDOptions> > hudOptionsObserver;
            std::shared_ptr<ftk::Observer<tl::TimeUnits> > timeUnitsObserver;
            std::shared_ptr<ftk::Observer<models::MouseSettings> > mouseSettingsObserver;
            std::shared_ptr<ftk::Observer<std::optional<ftk::V2I> > > pickObserver;
            std::shared_ptr<ftk::Observer<std::optional<ftk::Color4F> > > colorSampleObserver;
            std::shared_ptr<ftk::ListObserver<models::ReviewAnnotation> > annotationsObserver;

            enum class MouseMode
            {
                None,
                Shuttle,
                Draw,
                Erase
            };
            struct MouseData
            {
                MouseMode mode = MouseMode::None;
                std::optional<OTIO_NS::RationalTime> shuttleStart;
            };
            MouseData mouse;

            //! The stroke being drawn, in the image pixels of its source.
            models::ReviewStroke stroke;
            int strokeSource = -1;

            std::shared_ptr<models::FilesModel> filesModel;
            std::shared_ptr<models::AnnotationsModel> annotationsModel;
            std::shared_ptr<models::DrawModel> drawModel;
        };

        void Viewport::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::FilesModel>& filesModel,
            const std::shared_ptr<models::ColorModel>& colorModel,
            const std::shared_ptr<models::ViewportModel>& viewportModel,
            const std::shared_ptr<models::TimeUnitsModel>& timeUnitsModel,
            const std::shared_ptr<models::SettingsModel>& settingsModel,
            const std::shared_ptr<models::AnnotationsModel>& annotationsModel,
            const std::shared_ptr<models::DrawModel>& drawModel,
            const std::shared_ptr<ftk::SysLogModel>& sysLogModel,
            const std::shared_ptr<IWidget>& parent)
        {
            tl::ui::Viewport::_init(context, parent);
            FTK_P();

            setClipChildren(true);

            p.filesModel = filesModel;
            p.annotationsModel = annotationsModel;
            p.drawModel = drawModel;
            p.viewportModel = viewportModel;
            p.timeUnitsModel = timeUnitsModel;

            p.fileNameLabel = ftk::Label::create(context);
            p.fileNameLabel->setFont(ftk::FontType::Mono);
            p.fileNameLabel->setMarginRole(ftk::SizeRole::MarginSmall);

            p.cacheLabel = ftk::Label::create(context);
            p.cacheLabel->setFont(ftk::FontType::Mono);
            p.cacheLabel->setMarginRole(ftk::SizeRole::MarginSmall);

            p.timeLabel = ftk::Label::create(context);
            p.timeLabel->setFont(ftk::FontType::Mono);
            p.timeLabel->setMarginRole(ftk::SizeRole::MarginSmall);

            p.viewZoomLabel = ftk::Label::create(context);
            p.viewZoomLabel->setFont(ftk::FontType::Mono);
            p.viewZoomLabel->setMarginRole(ftk::SizeRole::MarginSmall);

            p.colorPickerSwatch = ftk::ColorSwatch::create(context);
            p.colorPickerSwatch->setVAlign(ftk::VAlign::Center);
            p.colorPickerLabel = ftk::Label::create(context);
            p.colorPickerLabel->setFont(ftk::FontType::Mono);
            p.colorPickerLabel->setMarginRole(ftk::SizeRole::MarginSmall);
            auto colorPickerLayout = ftk::HorizontalLayout::create(context, p.hudLayout);
            colorPickerLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.colorPickerSwatch->setParent(colorPickerLayout);
            p.colorPickerLabel->setParent(colorPickerLayout);

            p.infoLabel = ftk::Label::create(context);
            p.infoLabel->setFont(ftk::FontType::Mono);
            p.infoLabel->setMarginRole(ftk::SizeRole::MarginSmall);

            p.renderLabel = ftk::Label::create(context);
            p.renderLabel->setFont(ftk::FontType::Mono);
            p.renderLabel->setMarginRole(ftk::SizeRole::MarginSmall);

            p.hudWidgets[models::HUDItem::FileName] = p.fileNameLabel;
            p.hudWidgets[models::HUDItem::Cache] = p.cacheLabel;
            p.hudWidgets[models::HUDItem::Time] = p.timeLabel;
            p.hudWidgets[models::HUDItem::ViewZoom] = p.viewZoomLabel;
            p.hudWidgets[models::HUDItem::ColorPicker] = colorPickerLayout;
            p.hudWidgets[models::HUDItem::Info] = p.infoLabel;
            p.hudWidgets[models::HUDItem::Render] = p.renderLabel;

            p.hudLayout = ftk::VerticalLayout::create(context, shared_from_this());
            p.hudLayout->setMarginRole(ftk::SizeRole::MarginSmall);
            p.hudLayout->setSpacingRole(ftk::SizeRole::None);
            for (const auto i : models::getHUDPosEnums())
            {
                if (models::HUDPos::None == i)
                    continue;
                p.hudLayouts[i] = ftk::VerticalLayout::create(context);
                p.hudLayouts[i]->setMarginRole(ftk::SizeRole::MarginInside);
                p.hudLayouts[i]->setSpacingRole(ftk::SizeRole::None);
                p.hudLayouts[i]->setBackgroundRole(ftk::ColorRole::Overlay);
            }
            p.hudLayouts[models::HUDPos::TopLeft]->setVAlign(ftk::VAlign::Top);
            p.hudLayouts[models::HUDPos::TopRight]->setVAlign(ftk::VAlign::Top);
            p.hudLayouts[models::HUDPos::BottomLeft]->setVAlign(ftk::VAlign::Bottom);
            p.hudLayouts[models::HUDPos::BottomRight]->setVAlign(ftk::VAlign::Bottom);

            p.compareLabel = ftk::Label::create(context);
            p.compareLabel->setMarginRole(ftk::SizeRole::MarginSmall);
            p.compareLabel->setBackgroundRole(ftk::ColorRole::Overlay);
            p.compareLabel->setVisible(false);

            p.toastLabel = ftk::Label::create(context);
            p.toastLabel->setMarginRole(ftk::SizeRole::MarginSmall);
            p.toastLabel->setBackgroundRole(ftk::ColorRole::Overlay);
            p.toastLabel->setClipText(true);
            p.toastLabel->setVisible(false);

            auto topLayout = ftk::HorizontalLayout::create(context, p.hudLayout);
            topLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            topLayout->setVAlign(ftk::VAlign::Top);
            p.hudLayouts[models::HUDPos::TopLeft]->setParent(topLayout);
            auto spacer = ftk::Spacer::create(
                context, ftk::Orientation::Horizontal, topLayout);
            spacer->setStretch(ftk::Stretch::Expanding);
            p.hudLayouts[models::HUDPos::TopRight]->setParent(topLayout);

            spacer = ftk::Spacer::create(
                context, ftk::Orientation::Vertical, p.hudLayout);
            spacer->setStretch(ftk::Stretch::Expanding);

            auto noBLayout = ftk::HorizontalLayout::create(context, p.hudLayout);
            noBLayout->setVAlign(ftk::VAlign::Center);
            spacer = ftk::Spacer::create(
                context, ftk::Orientation::Horizontal, noBLayout);
            spacer->setStretch(ftk::Stretch::Expanding);
            p.compareLabel->setParent(noBLayout);
            spacer = ftk::Spacer::create(
                context, ftk::Orientation::Horizontal, noBLayout);
            spacer->setStretch(ftk::Stretch::Expanding);
            spacer = ftk::Spacer::create(
                context, ftk::Orientation::Vertical, p.hudLayout);
            spacer->setStretch(ftk::Stretch::Expanding);

            auto bottomLayout = ftk::VerticalLayout::create(context, p.hudLayout);
            bottomLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            auto toastLayout = ftk::HorizontalLayout::create(context, bottomLayout);
            spacer = ftk::Spacer::create(
                context, ftk::Orientation::Horizontal, toastLayout);
            spacer->setStretch(ftk::Stretch::Expanding);
            p.toastLabel->setParent(toastLayout);
            spacer = ftk::Spacer::create(
                context, ftk::Orientation::Horizontal, toastLayout);
            spacer->setStretch(ftk::Stretch::Expanding);
            auto bottomRowLayout = ftk::HorizontalLayout::create(context, bottomLayout);
            bottomRowLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            bottomRowLayout->setVAlign(ftk::VAlign::Bottom);
            p.hudLayouts[models::HUDPos::BottomLeft]->setParent(bottomRowLayout);
            spacer = ftk::Spacer::create(
                context, ftk::Orientation::Horizontal, bottomRowLayout);
            spacer->setStretch(ftk::Stretch::Expanding);
            p.hudLayouts[models::HUDPos::BottomRight]->setParent(bottomRowLayout);

            p.toastTimer = ftk::Timer::create(context);

            p.fpsObserver = ftk::Observer<double>::create(
                observeFPS(),
                [this](double value)
                {
                    _p->fps = value;
                    _hudUpdate();
                });

            p.droppedFramesObserver = ftk::Observer<size_t>::create(
                observeDroppedFrames(),
                [this](size_t value)
                {
                    _p->droppedFrames = value;
                    _hudUpdate();
                });

            p.aObserver = ftk::Observer<std::shared_ptr<models::FilesModelItem> >::create(
                filesModel->observeA(),
                [this](const std::shared_ptr<models::FilesModelItem>& value)
                {
                    _p->a = value;
                    _compareUpdate();
                });

            p.bObserver = ftk::ListObserver<std::shared_ptr<models::FilesModelItem> >::create(
                filesModel->observeB(),
                [this](const std::vector<std::shared_ptr<models::FilesModelItem> >& value)
                {
                    _p->b = value;
                    _compareUpdate();
                });

            p.compareOptionsObserver = ftk::Observer<tl::CompareOptions>::create(
                filesModel->observeCompareOptions(),
                [this](const tl::CompareOptions& value)
                {
                    _p->compare = value.compare;
                    setCompareOptions(value);
                    _compareUpdate();
                });

            p.drawEnabledObserver = ftk::Observer<bool>::create(
                drawModel->observeEnabled(),
                [this](bool)
                {
                    _cursorUpdate();
                });

            p.compareTimeObserver = ftk::Observer<tl::CompareTime>::create(
                filesModel->observeCompareTime(),
                [this](tl::CompareTime value)
                {
                    _p->compareTime = value;
                    _compareUpdate();
                });

            // The options as written, not the resolved ones: the per item
            // display options carry each file's resolved input, so a file
            // that resolves nothing falls back to what the user chose
            // rather than to whatever the active file resolved to.
            p.ocioOptionsObserver = ftk::Observer<tl::OCIOOptions>::create(
                colorModel->observeOCIOOptions(),
                [this](const tl::OCIOOptions& value)
                {
                   setOCIOOptions(value);
                });

            p.resolvedInputsObserver = ftk::Observer<std::vector<std::string> >::create(
                colorModel->observeResolvedInputs(),
                [this](const std::vector<std::string>& value)
                {
                    _p->ocioInputs = value;
                    _videoUpdate();
                });

            // Layers of a timeline resolve their own input color spaces --
            // each clip of an OTIO file is its own media -- but only when
            // the input is automatic; one the user chose applies to every
            // layer.
            
            setOCIOInputResolver(
                [colorModel](const std::string& path, const ftk::ImageTags& tags)
                {
                    return colorModel->getOCIOOptions().input.empty() ?
                        colorModel->resolveInput(path, tags) :
                        std::string();
                });

            p.lutOptionsObserver = ftk::Observer<tl::LUTOptions>::create(
                colorModel->observeLUTOptions(),
                [this](const tl::LUTOptions& value)
                {
                   setLUTOptions(value);
                });

            p.imageOptionsObserver = ftk::Observer<ftk::ImageOptions>::create(
                viewportModel->observeImageOptions(),
                [this](const ftk::ImageOptions& value)
                {
                    _p->imageOptions = value;
                    _videoUpdate();
                });

            p.displayOptionsObserver = ftk::Observer<tl::DisplayOptions>::create(
                viewportModel->observeDisplayOptions(),
                [this](const tl::DisplayOptions& value)
                {
                    _p->displayOptions = value;
                    _videoUpdate();
                    // The heads up display says what is being rendered, which
                    // the aspect ratio changes. Without this it would only
                    // catch up when something else refreshes it -- which is
                    // every frame while playing, and nothing at all while
                    // stopped.
                    _hudUpdate();
                });

            p.bgOptionsObserver = ftk::Observer<tl::BackgroundOptions>::create(
                viewportModel->observeBackgroundOptions(),
                [this](const tl::BackgroundOptions& value)
                {
                    setBackgroundOptions(value);
                });

            p.fgOptionsObserver = ftk::Observer<tl::ForegroundOptions>::create(
                viewportModel->observeForegroundOptions(),
                [this](const tl::ForegroundOptions& value)
                {
                    setForegroundOptions(value);
                });

            p.colorBufferObserver = ftk::Observer<ftk::gl::TextureType>::create(
                viewportModel->observeColorBuffer(),
                [this](ftk::gl::TextureType value)
                {
                    setColorBuffer(value);
                    _hudUpdate();
                });

            p.viewZoomObserver = ftk::Observer<double>::create(
                observeZoom(),
                [this](double value)
                {
                    _p->viewZoom = value;
                    _hudUpdate();
                });

            p.messagesObserver = ftk::ListObserver<ftk::LogItem>::create(
                sysLogModel->observeMessages(),
                [this](const std::vector<ftk::LogItem>& value)
                {
                    FTK_P();
                    if (value.empty())
                    {
                        // Cleared, so there is nothing left to be showing.
                        p.toastLabel->setText(std::string());
                        _toastUpdate();
                        return;
                    }
                    // Errors only. The list carries warnings as well, and the
                    // status bar shows both; drawing over the image is for
                    // what stopped something from working, not for what is
                    // merely worth knowing. A warning arriving is also not a
                    // reason to cut short an error that is still up.
                    if (ftk::LogType::Error != value.back().type)
                    {
                        return;
                    }
                    // The message alone: it has just appeared, and the space
                    // over the image is better spent on what went wrong.
                    p.toastLabel->setText(
                        ftk::elide(
                            ftk::getLabel(value.back(), ftk::LogLabel::Message),
                            toastTextLength));
                    _toastUpdate();
                    if (!p.toastLabel->getText().empty())
                    {
                        p.toastTimer->start(
                            toastTimeout,
                            [this]
                            {
                                _p->toastLabel->setText(std::string());
                                _toastUpdate();
                            });
                    }
                });

            p.hudOptionsObserver = ftk::Observer<models::HUDOptions>::create(
                viewportModel->observeHUDOptions(),
                [this](const models::HUDOptions& value)
                {
                    _p->hudOptions = value;
                    _hudUpdate();
                    _hudLayout();
                });

            p.timeUnitsObserver = ftk::Observer<tl::TimeUnits>::create(
                timeUnitsModel->observeTimeUnits(),
                [this](tl::TimeUnits value)
                {
                    _hudUpdate();
                });

            // The overlay is drawn straight from the model, so any change to it
            // has to ask for a redraw. Drawing does that from the mouse events,
            // but undo, redo and "Clear Drawing" only touch the model: without
            // this the frame keeps showing stale strokes until some unrelated
            // event happens to repaint the window.
            p.annotationsObserver = ftk::ListObserver<models::ReviewAnnotation>::create(
                annotationsModel->observeAnnotations(),
                [this](const std::vector<models::ReviewAnnotation>&)
                {
                    setDrawUpdate();
                });

            p.mouseSettingsObserver = ftk::Observer<models::MouseSettings>::create(
                settingsModel->observeMouse(),
                [this](const models::MouseSettings& value)
                {
                    FTK_P();
                    auto i = value.bindings.find(models::MouseAction::PanView);
                    setPanBinding(
                        i != value.bindings.end() ? i->second.button : ftk::MouseButton::None,
                        i != value.bindings.end() ? i->second.modifier : ftk::KeyModifier::None);
                    i = value.bindings.find(models::MouseAction::CompareWipe);
                    setWipeBinding(
                        i != value.bindings.end() ? i->second.button : ftk::MouseButton::None,
                        i != value.bindings.end() ? i->second.modifier : ftk::KeyModifier::None);
                    i = value.bindings.find(models::MouseAction::Pick);
                    setPickBinding(
                        i != value.bindings.end() ? i->second.button : ftk::MouseButton::None,
                        i != value.bindings.end() ? i->second.modifier : ftk::KeyModifier::None);
                    i = value.bindings.find(models::MouseAction::FrameShuttle);
                    p.frameShuttleBinding = i != value.bindings.end() ? i->second : models::MouseActionBinding();
                    p.frameShuttleScale = value.frameShuttleScale;
                });

            p.pickObserver = ftk::Observer<std::optional<ftk::V2I> >::create(
                observePick(),
                [this](const std::optional<ftk::V2I>&)
                {
                    _hudUpdate();
                });

            p.colorSampleObserver = ftk::Observer<std::optional<ftk::Color4F> >::create(
                observeColorSample(),
                [this](const std::optional<ftk::Color4F>&)
                {
                    _hudUpdate();
                });
        }

        Viewport::Viewport() :
            _p(new Private)
        {}

        Viewport::~Viewport()
        {}

        std::shared_ptr<Viewport> Viewport::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::FilesModel>& filesModel,
            const std::shared_ptr<models::ColorModel>& colorModel,
            const std::shared_ptr<models::ViewportModel>& viewportModel,
            const std::shared_ptr<models::TimeUnitsModel>& timeUnitsModel,
            const std::shared_ptr<models::SettingsModel>& settingsModel,
            const std::shared_ptr<models::AnnotationsModel>& annotationsModel,
            const std::shared_ptr<models::DrawModel>& drawModel,
            const std::shared_ptr<ftk::SysLogModel>& sysLogModel,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<Viewport>(new Viewport);
            out->_init(
                context,
                filesModel,
                colorModel,
                viewportModel,
                timeUnitsModel,
                settingsModel,
                annotationsModel,
                drawModel,
                sysLogModel,
                parent);
            return out;
        }

        void Viewport::setPlayer(const std::shared_ptr<tl::Player>& player)
        {
            tl::ui::Viewport::setPlayer(player);
            FTK_P();
            if (player)
            {
                p.path = player->getPath();

                // The information describes the media reference being read, so
                // it is refreshed when the key changes. The observer also
                // reports the current key, which covers the new player.
                p.mediaReferenceKeyObserver = ftk::Observer<std::string>::create(
                    player->observeMediaReferenceKey(),
                    [this, player](const std::string&)
                    {
                        _p->ioInfo = player->getIOInfo();
                        // Panning and zooming move an image around the view.
                        // Media with no video has none, so the view has
                        // nothing to move and the wheel would change a zoom
                        // that shows nothing.
                        setInputEnabled(!_p->ioInfo.video.empty());
                        _hudUpdate();
                    });

                p.currentTimeObserver = ftk::Observer<OTIO_NS::RationalTime>::create(
                    player->observeCurrentTime(),
                    [this](const OTIO_NS::RationalTime& value)
                    {
                        _p->currentTime = value;
                        _hudUpdate();
                        // Annotations are per frame, so the overlay changes.
                        setDrawUpdate();
                    });

                p.videoObserver = ftk::ListObserver<tl::VideoFrame>::create(
                    player->observeCurrentVideo(),
                    [this](const std::vector<tl::VideoFrame>& value)
                    {
                        FTK_P();
                        p.videoFramesSize = value.size();
                        p.videoFrames = value;
                        _compareUpdate();
                        p.missing = false;
                        p.heldFrom.reset();
                        // The first source, which is the one the time in the
                        // heads up display is about.
                        if (!value.empty())
                        {
                            for (const auto& layer : value.front().layers)
                            {
                                if (layer.missing)
                                {
                                    p.missing = true;
                                    if (layer.heldFrom.has_value())
                                    {
                                        p.heldFrom = layer.heldFrom;
                                    }
                                }
                            }
                        }
                        _videoUpdate();
                        _hudUpdate();
                    });

                p.cacheObserver = ftk::Observer<tl::PlayerCacheInfo>::create(
                    player->observeCacheInfo(),
                    [this](const tl::PlayerCacheInfo& value)
                    {
                        _p->cacheInfo = value;
                        _hudUpdate();
                    });
            }
            else
            {
                p.path = ftk::Path();
                p.ioInfo = tl::IOInfo();
                setInputEnabled(false);
                p.mediaReferenceKeyObserver.reset();
                p.currentTime.reset();
                p.videoFrames.clear();
                p.currentTimeObserver.reset();
                p.videoObserver.reset();
                p.cacheInfo = tl::PlayerCacheInfo();
                p.cacheObserver.reset();
                _hudUpdate();
            }
        }

        void Viewport::setHUDActive(bool value)
        {
            FTK_P();
            if (value == p.hudActive)
                return;
            p.hudActive = value;
            _hudLayout();
        }

        void Viewport::setToastActive(bool value)
        {
            FTK_P();
            if (value == p.toastActive)
                return;
            p.toastActive = value;
            _toastUpdate();
        }

        ftk::Size2I Viewport::getSizeHint() const
        {
            return _p->hudLayout->getSizeHint();
        }

        void Viewport::setGeometry(const ftk::Box2I& value)
        {
            tl::ui::Viewport::setGeometry(value);
            FTK_P();
            p.hudLayout->setGeometry(value);
        }

        void Viewport::mouseMoveEvent(ftk::MouseMoveEvent& event)
        {
            tl::ui::Viewport::mouseMoveEvent(event);
            FTK_P();
            switch (p.mouse.mode)
            {
            case Private::MouseMode::Draw:
                _drawContinue(event.pos - getGeometry().min);
                break;
            case Private::MouseMode::Erase:
                _erase(event.pos - getGeometry().min);
                break;
            case Private::MouseMode::Shuttle:
                if (auto player = getPlayer())
                {
                    // The mode is taken on the press whether or not there was
                    // a player to read a position from, so the two can
                    // disagree and the shuttle has nothing to move from.
                    if (p.mouse.shuttleStart.has_value())
                    {
                        const OTIO_NS::RationalTime offset = OTIO_NS::RationalTime(
                            (event.pos.x - _getMousePressPos().x) * .05F * p.frameShuttleScale,
                            p.mouse.shuttleStart->rate()).round();
                        const OTIO_NS::TimeRange& timeRange = player->getTimeRange();
                        OTIO_NS::RationalTime t = *p.mouse.shuttleStart + offset;
                        if (t < timeRange.start_time())
                        {
                            t = timeRange.end_time_exclusive() - (timeRange.start_time() - t);
                        }
                        else if (t > timeRange.end_time_exclusive())
                        {
                            t = timeRange.start_time() + (t - timeRange.end_time_exclusive());
                        }
                        player->seek(t);
                    }
                }
                break;
            default: break;
            }
        }

        void Viewport::mousePressEvent(ftk::MouseClickEvent& event)
        {
            tl::ui::Viewport::mousePressEvent(event);
            FTK_P();
            // Shuttling is ours rather than the base class's, so it needs
            // this test of its own; without it it would go on working in a
            // view that takes no input.
            if (!isInputEnabled())
            {
                return;
            }

            // Drawing owns the plain left button while it is enabled, which is
            // why it is an explicit mode: it displaces the frame shuttle.
            if (p.drawModel->isEnabled() &&
                ftk::MouseButton::Left == event.button &&
                0 == event.modifiers)
            {
                // Claim the button, as the shuttle does below. Without this
                // the release never reaches us: the stroke is never
                // committed, keeps growing on every move, and the undo that
                // follows has to copy all of it.
                event.accept = true;
                takeKeyFocus();
                const ftk::Box2I& g = getGeometry();
                const ftk::V2I pos = event.pos - g.min;
                if (models::DrawTool::Eraser == p.drawModel->getTool())
                {
                    p.mouse.mode = Private::MouseMode::Erase;
                    _erase(pos);
                }
                else
                {
                    p.mouse.mode = Private::MouseMode::Draw;
                    _drawBegin(pos);
                }
                return;
            }

            if (p.frameShuttleBinding.button == event.button &&
                ftk::checkKeyModifier(p.frameShuttleBinding.modifier, event.modifiers))
            {
                // The base class only claims the buttons bound to its own
                // actions, so claim ours here; an unbound button is left
                // free to open a context menu.
                event.accept = true;
                takeKeyFocus();
                p.mouse.mode = Private::MouseMode::Shuttle;
                if (auto player = getPlayer())
                {
                    player->stop();
                    p.mouse.shuttleStart = player->getCurrentTime();
                }
            }
        }

        void Viewport::mouseReleaseEvent(ftk::MouseClickEvent& event)
        {
            tl::ui::Viewport::mouseReleaseEvent(event);
            FTK_P();
            if (Private::MouseMode::Draw == p.mouse.mode)
            {
                // Commit the stroke as one undoable step.
                _drawEnd();
            }
            p.mouse = Private::MouseData();
        }

        void Viewport::mouseEnterEvent(ftk::MouseEnterEvent& event)
        {
            tl::ui::Viewport::mouseEnterEvent(event);
            _cursorUpdate();
        }

        void Viewport::mouseLeaveEvent()
        {
            tl::ui::Viewport::mouseLeaveEvent();
            _cursorUpdate();
        }

        void Viewport::_cursorUpdate()
        {
            FTK_P();
            if (auto window = getWindow())
            {
                window->setCursor(
                    _isMouseInside() && p.drawModel->isEnabled() ?
                    ftk::CursorShape::Crosshair :
                    ftk::CursorShape::Arrow);
            }
        }

        std::vector<ftk::Box2I> Viewport::_sourceBoxes() const
        {
            FTK_P();
            // Mirror what tl::ui::Viewport uses when it draws, so the hit test
            // and the overlay land exactly where the image does.
            return tl::getBoxes(
                getCompareOptions(),
                p.displayOptions.aspectRatio,
                p.videoFrames);
        }

        bool Viewport::_sourceShown(int index) const
        {
            FTK_P();
            return tl::isShown(p.compare, static_cast<size_t>(index));
        }

        Viewport::SourceHit Viewport::_hitTest(const ftk::V2I& widgetPos) const
        {
            FTK_P();
            SourceHit out;
            // Widget -> render space: the inverse of translate(viewPos) * scale.
            const double zoom = getZoom();
            if (zoom <= 0.0)
            {
                return out;
            }
            const ftk::V2I& viewPos = getViewPos();
            const ftk::V2F render(
                static_cast<float>((widgetPos.x - viewPos.x) / zoom),
                static_cast<float>((widgetPos.y - viewPos.y) / zoom));

            const auto boxes = _sourceBoxes();
            for (size_t i = 0; i < boxes.size() && i < p.videoFrames.size(); ++i)
            {
                if (!_sourceShown(static_cast<int>(i)))
                {
                    continue;
                }
                const ftk::Box2I& box = boxes[i];
                if (render.x < box.min.x || render.x > box.max.x ||
                    render.y < box.min.y || render.y > box.max.y ||
                    box.w() <= 0 || box.h() <= 0)
                {
                    continue;
                }
                const auto& video = p.videoFrames[i];
                if (video.layers.empty() || !video.layers[0].image)
                {
                    continue;
                }
                const ftk::Size2I imageSize = video.layers[0].image->getSize();
                if (imageSize.w <= 0 || imageSize.h <= 0)
                {
                    continue;
                }
                // Render space -> the source's own pixels.
                out.index = static_cast<int>(i);
                out.pos = ftk::V2F(
                    (render.x - box.min.x) * imageSize.w / static_cast<float>(box.w()),
                    (render.y - box.min.y) * imageSize.h / static_cast<float>(box.h()));
                // The mirror flips the image inside its box, so the same
                // flip takes the hit back to the image's own pixels. Strokes
                // are stored unmirrored, and follow the image when the
                // mirror changes.
                if (p.displayOptions.mirror.x)
                {
                    out.pos.x = imageSize.w - out.pos.x;
                }
                if (p.displayOptions.mirror.y)
                {
                    out.pos.y = imageSize.h - out.pos.y;
                }
                out.scale = box.w() / static_cast<float>(imageSize.w);
                break;
            }
            return out;
        }

        ftk::V2F Viewport::_imageToWidget(int index, const ftk::V2F& imagePos) const
        {
            FTK_P();
            const auto boxes = _sourceBoxes();
            if (index < 0 ||
                index >= static_cast<int>(boxes.size()) ||
                index >= static_cast<int>(p.videoFrames.size()))
            {
                return ftk::V2F();
            }
            const ftk::Box2I& box = boxes[index];
            const auto& video = p.videoFrames[index];
            if (video.layers.empty() || !video.layers[0].image)
            {
                return ftk::V2F();
            }
            const ftk::Size2I imageSize = video.layers[0].image->getSize();
            if (imageSize.w <= 0 || imageSize.h <= 0)
            {
                return ftk::V2F();
            }
            // The inverse of the flip in _hitTest: stored image pixels back
            // to where the mirror shows them.
            ftk::V2F pos = imagePos;
            if (p.displayOptions.mirror.x)
            {
                pos.x = imageSize.w - pos.x;
            }
            if (p.displayOptions.mirror.y)
            {
                pos.y = imageSize.h - pos.y;
            }
            const float renderX = box.min.x + pos.x * box.w() / static_cast<float>(imageSize.w);
            const float renderY = box.min.y + pos.y * box.h() / static_cast<float>(imageSize.h);
            const double zoom = getZoom();
            const ftk::V2I& viewPos = getViewPos();
            return ftk::V2F(
                static_cast<float>(viewPos.x + renderX * zoom),
                static_cast<float>(viewPos.y + renderY * zoom));
        }

        void Viewport::_drawBegin(const ftk::V2I& widgetPos)
        {
            FTK_P();
            const SourceHit hit = _hitTest(widgetPos);
            if (hit.index < 0)
            {
                return;
            }
            {
                const auto& drawModel = p.drawModel;
                p.strokeSource = hit.index;
                p.stroke = models::ReviewStroke();
                p.stroke.color = drawModel->getColor();
                p.stroke.width = drawModel->getSize();
                p.stroke.points.push_back(hit.pos);
                setDrawUpdate();
            }
        }

        void Viewport::_drawContinue(const ftk::V2I& widgetPos)
        {
            FTK_P();
            if (p.strokeSource < 0)
            {
                return;
            }
            const SourceHit hit = _hitTest(widgetPos);
            // Keep the stroke on the source it started on, so dragging across a
            // comparison boundary does not tear it in two.
            if (hit.index != p.strokeSource)
            {
                return;
            }
            if (!p.stroke.points.empty())
            {
                const ftk::V2F& last = p.stroke.points.back();
                const float dx = hit.pos.x - last.x;
                const float dy = hit.pos.y - last.y;
                // Drop points the mouse barely moved, to keep strokes compact.
                if (dx * dx + dy * dy < .25F)
                {
                    return;
                }
            }
            p.stroke.points.push_back(hit.pos);
            setDrawUpdate();
        }

        void Viewport::_drawEnd()
        {
            FTK_P();
            if (p.strokeSource >= 0 && !p.stroke.points.empty())
            {
                {
                    const auto& active = p.filesModel->getActive();
                    if (p.strokeSource < static_cast<int>(active.size()))
                    {
                        p.annotationsModel->addStroke(
                            active[p.strokeSource]->id,
                            *p.currentTime,
                            p.stroke);
                    }
                }
            }
            p.stroke = models::ReviewStroke();
            p.strokeSource = -1;
            setDrawUpdate();
        }

        void Viewport::_erase(const ftk::V2I& widgetPos)
        {
            FTK_P();
            const SourceHit hit = _hitTest(widgetPos);
            if (hit.index < 0)
            {
                return;
            }
            {
                const auto& active = p.filesModel->getActive();
                if (hit.index < static_cast<int>(active.size()))
                {
                    // The eraser removes whole strokes it touches; its reach
                    // follows the tool size.
                    p.annotationsModel->eraseStrokes(
                        active[hit.index]->id,
                        *p.currentTime,
                        hit.pos,
                        p.drawModel->getSize());
                }
            }
        }

        void Viewport::drawEvent(const ftk::Box2I& drawRect, const ftk::DrawEvent& event)
        {
            tl::ui::Viewport::drawEvent(drawRect, event);
            FTK_P();

            const ftk::Box2I& g = getGeometry();
            const double zoom = getZoom();
            const auto& active = p.filesModel->getActive();

            // Keep the overlay inside the viewport: a stroke zoomed past the
            // edges would otherwise be painted over the panels and toolbars.
            const bool clipRectEnabledPrev = event.render->getClipRectEnabled();
            const ftk::Box2I clipRectPrev = event.render->getClipRect();
            event.render->setClipRectEnabled(true);
            event.render->setClipRect(ftk::intersect(g, clipRectPrev));

            // Draw a stroke, converting the stored image pixels back to the
            // screen so it tracks the zoom, the pan and the comparison mode.
            auto drawStroke = [this, &event, &g, zoom](
                int index,
                const models::ReviewStroke& stroke,
                float scale)
            {
                if (stroke.points.empty())
                {
                    return;
                }
                // To the screen first, then smooth and thicken there, so the
                // curve stays smooth at any zoom.
                std::vector<ftk::V2F> path;
                path.reserve(stroke.points.size());
                for (const auto& point : stroke.points)
                {
                    ftk::V2F p = _imageToWidget(index, point);
                    p.x += g.min.x;
                    p.y += g.min.y;
                    path.push_back(p);
                }
                const float width = std::max(1.F, stroke.width * scale * static_cast<float>(zoom));
                // Thin the captured points before smoothing, then subdivide:
                // clustered controls are what make the spline overshoot.
                event.render->drawMesh(
                    strokeMesh(smoothPath(simplifyPath(path, 3.F), 12), width),
                    stroke.color);
            };

            const auto boxes = _sourceBoxes();
            auto sourceScale = [this, &boxes](int index) -> float
            {
                FTK_P();
                if (index < 0 ||
                    index >= static_cast<int>(boxes.size()) ||
                    index >= static_cast<int>(p.videoFrames.size()))
                {
                    return 1.F;
                }
                const auto& video = p.videoFrames[index];
                if (video.layers.empty() || !video.layers[0].image)
                {
                    return 1.F;
                }
                const int w = video.layers[0].image->getSize().w;
                return w > 0 ? boxes[index].w() / static_cast<float>(w) : 1.F;
            };

            // The strokes already committed on this frame.
            const auto& annotations = p.annotationsModel->getAnnotations();
            for (const auto& annotation : annotations)
            {
                if (!models::sameTime(annotation.time, p.currentTime))
                {
                    continue;
                }
                int index = -1;
                for (size_t i = 0; i < active.size(); ++i)
                {
                    if (active[i]->id == annotation.sourceId)
                    {
                        index = static_cast<int>(i);
                        break;
                    }
                }
                if (index < 0 || !_sourceShown(index))
                {
                    continue;
                }
                const float scale = sourceScale(index);
                for (const auto& stroke : annotation.strokes)
                {
                    drawStroke(index, stroke, scale);
                }
            }

            // The stroke currently under the cursor.
            if (p.strokeSource >= 0)
            {
                drawStroke(p.strokeSource, p.stroke, sourceScale(p.strokeSource));
            }

            event.render->setClipRectEnabled(clipRectEnabledPrev);
            event.render->setClipRect(clipRectPrev);
        }

        void Viewport::_videoUpdate()
        {
            FTK_P();
            std::vector<ftk::ImageOptions> imageOptionsList;
            std::vector<tl::DisplayOptions> displayOptionsList;
            for (size_t i = 0; i < p.videoFramesSize; ++i)
            {
                imageOptionsList.push_back(p.imageOptions);
                displayOptionsList.push_back(p.displayOptions);
                // The input color space resolved for this item's file, so
                // a comparison of files in different color spaces shows
                // each of them correctly.
                displayOptionsList.back().ocioInput =
                    i < p.ocioInputs.size() ? p.ocioInputs[i] : std::string();
            }
            setImageOptions(imageOptionsList);
            setDisplayOptions(displayOptionsList);
        }

        void Viewport::_compareUpdate()
        {
            FTK_P();
            // Everything but A needs a B file that is not the A file. Neither
            // draws anything, and an empty picture is the one thing a black
            // frame, an unreadable file and a comparison out of sync all look
            // like as well.
            std::string s;
            if (p.compare != tl::Compare::None)
            {
                if (p.videoFramesSize < 2)
                {
                    if (p.b.empty())
                    {
                        s = "No B file selected";
                    }
                    else if (p.a && p.a->videoLayers.empty())
                    {
                        // The player is built from "A", and one with no
                        // video runs no video at all -- the comparison has
                        // nothing to composite, however real the B file is.
                        s = "A has no video to compare";
                    }
                    // Otherwise the frames are still loading, which is not
                    // worth an announcement.
                }
                else if (p.a && !p.b.empty())
                {
                    // Only when there is nothing else in B: a file compared
                    // with itself alongside others still has something to
                    // show.
                    const bool allA = std::all_of(
                        p.b.begin(),
                        p.b.end(),
                        [this](const std::shared_ptr<models::FilesModelItem>& i)
                        {
                            return i == _p->a;
                        });
                    if (allA)
                    {
                        s = "A and B are the same file";
                    }
                    else if (tl::CompareTime::Absolute == p.compareTime &&
                        p.a->timeRange.has_value())
                    {
                        // Timecode sync with no timecode in common: every B
                        // frame maps outside its file, so B never draws.
                        // Decided from the ranges rather than the missing
                        // frame, which is also what a frame still loading
                        // looks like.
                        const bool noOverlap = std::all_of(
                            p.b.begin(),
                            p.b.end(),
                            [this](const std::shared_ptr<models::FilesModelItem>& i)
                            {
                                return i->timeRange.has_value() &&
                                    !i->timeRange->intersects(
                                        *_p->a->timeRange);
                            });
                        if (noOverlap)
                        {
                            s = "A and B timecodes do not overlap";
                        }
                    }
                }
            }
            p.compareLabel->setText(s);
            p.compareLabel->setVisible(!s.empty());
            ftk::setScreenshotTag(p.compareLabel, !s.empty() ? "View.Compare" : "");
        }

        void Viewport::_hudUpdate()
        {
            FTK_P();

            std::string s = p.path.getFileName();
            p.fileNameLabel->setText(!s.empty() ? s : "(No file)");
            ftk::setScreenshotTag(p.fileNameLabel, "View.HUD.FileName");

            std::vector<std::string> info;
            if (!p.ioInfo.video.empty())
            {
                info.push_back(std::string(ftk::Format("V: {0}").
                    arg(ftk::getLabel(p.ioInfo.video[0]))));
            }
            if (p.ioInfo.audio.isValid())
            {
                info.push_back(std::string(ftk::Format("A: {0}").
                    arg(tl::getLabel(p.ioInfo.audio, true))));
            }
            p.infoLabel->setText(ftk::join(info, ", "));
            p.infoLabel->setVisible(!info.empty());
            ftk::setScreenshotTag(p.infoLabel, "View.HUD.Info");

            // What is actually rendered, which the pixel aspect ratio and the
            // aspect ratio override can both move away from the media size
            // reported above. The effective pixel aspect ratio is taken back
            // out of the render size rather than read from either source, so
            // it holds whether it came from the media or from an override.
            s = std::string();
            if (!p.ioInfo.video.empty())
            {
                const ftk::ImageInfo& videoInfo = p.ioInfo.video[0];
                const ftk::Size2I renderSize = tl::getRenderSize(
                    videoInfo,
                    p.displayOptions.aspectRatio);
                if (renderSize.isValid() && videoInfo.size.w > 0)
                {
                    const float pixelAspectRatio =
                        renderSize.w / static_cast<float>(videoInfo.size.w);
                    s = ftk::Format("Render: {0}x{1}:{2}").
                        arg(renderSize.w).
                        arg(renderSize.h).
                        arg(ftk::aspectRatio(renderSize), 2);
                    // Square pixels are the common case and add nothing.
                    if (std::fabs(pixelAspectRatio - 1.F) > 0.001F)
                    {
                        s += ftk::Format(", PAR: {0}").
                            arg(pixelAspectRatio, 2);
                    }
                }
            }
            p.renderLabel->setText(s);
            p.renderLabel->setVisible(!s.empty());
            ftk::setScreenshotTag(p.renderLabel, "View.HUD.Render");

            s = std::string();
            if (auto timeUnitsModel = p.timeUnitsModel.lock())
            {
                s = timeUnitsModel->getLabel(p.currentTime);
            }
            std::string missing;
            if (p.missing)
            {
                // Said rather than only drawn, so which frame is standing in
                // is known and not merely that one is.
                missing = p.heldFrom.has_value() ?
                    ftk::Format(", held from {0}").
                        arg(p.heldFrom.value()).str() :
                    ", missing";
            }
            // Frames per second and frames dropped are about video, and a
            // file without any has neither -- reporting none and an
            // ever-growing count of what was never going to arrive reads as
            // a fault rather than as an absence.
            std::string timeText = ftk::Format("Time: {0}{1}").
                arg(s).
                arg(missing);
            if (!p.ioInfo.video.empty())
            {
                timeText = ftk::Format("Time: {0}, {1} FPS, {2} dropped{3}").
                    arg(s).
                    arg(p.fps, 2, 6).
                    arg(static_cast<int>(p.droppedFrames), 3).
                    arg(missing);
            }
            p.timeLabel->setText(timeText);
            ftk::setScreenshotTag(p.timeLabel, "View.HUD.Time");

            p.viewZoomLabel->setText(ftk::Format("Zoom: {0}").
                arg(p.viewZoom, 2, 6));
            ftk::setScreenshotTag(p.viewZoomLabel, "View.HUD.ViewZoom");

            const auto& colorSample = observeColorSample()->get();
            const auto& pick = observePick()->get();
            p.colorPickerSwatch->setColor(
                colorSample.has_value() ? colorSample.value() : ftk::Color4F());
            // The HUD sits under the pointer, so a line that changes width
            // as values come and go moves exactly where the eye is. Both
            // forms are built from the same layout and field widths, so the
            // line keeps its size whether or not there is a sample -- which
            // is what happens every time the pointer leaves the image. The
            // widths hold a sign and two digits, so the ordinary zero to one
            // range and a little either side of it do not move the line
            // either; beyond that the field grows, as it did before.
            const int colorWidth = 5;
            const int pickWidth = 4;
            const std::string colorPickerFormat =
                "Color: {0} {1} {2} {3}, Pixel: {4}, {5}";
            std::string colorPickerText =
                ftk::Format(colorPickerFormat).
                arg("-", colorWidth).
                arg("-", colorWidth).
                arg("-", colorWidth).
                arg("-", colorWidth).
                arg("-", pickWidth).
                arg("-", pickWidth);
            if (colorSample.has_value() && pick.has_value())
            {
                colorPickerText =
                    ftk::Format(colorPickerFormat).
                    arg(colorSample.value().r, 2, colorWidth).
                    arg(colorSample.value().g, 2, colorWidth).
                    arg(colorSample.value().b, 2, colorWidth).
                    arg(colorSample.value().a, 2, colorWidth).
                    arg(pick.value().x, pickWidth).
                    arg(pick.value().y, pickWidth);
            }
            p.colorPickerLabel->setText(colorPickerText);
            ftk::setScreenshotTag(p.colorPickerLabel, "View.HUD.ColorPicker");
            ftk::setScreenshotTag(p.colorPickerSwatch, "View.HUD.ColorPickerSwatch");

            std::vector<std::string> cache;
            if (!p.ioInfo.video.empty())
            {
                cache.push_back(std::string(ftk::Format("{0}% V").
                    arg(static_cast<int>(p.cacheInfo.videoPercentage), 3)));
            }
            if (p.ioInfo.audio.isValid())
            {
                cache.push_back(std::string(ftk::Format("{0}% A").
                    arg(static_cast<int>(p.cacheInfo.audioPercentage), 3)));
            }
            s = !cache.empty() ?
                std::string(ftk::Format("Cache: {0}").arg(ftk::join(cache, ", "))) :
                std::string();
            p.cacheLabel->setText(s);
            p.cacheLabel->setVisible(!s.empty());
            ftk::setScreenshotTag(p.cacheLabel, "View.HUD.Cache");
        }

        void Viewport::_toastUpdate()
        {
            FTK_P();
            const bool visible =
                p.toastActive && !p.toastLabel->getText().empty();
            p.toastLabel->setVisible(visible);
            // Tagged only while it is up, the same as the compare label, so
            // that a capture says whether anything was drawn over the image
            // rather than only what it would have said.
            ftk::setScreenshotTag(p.toastLabel, visible ? "View.Toast" : "");
        }

        void Viewport::_hudLayout()
        {
            FTK_P();
            auto viewportModel = p.viewportModel.lock();
            const auto options = viewportModel->getHUDOptions();
            for (const auto& i : options.items)
            {
                p.hudWidgets[i.first]->setParent(i.second != models::HUDPos::None ?
                    p.hudLayouts[i.second] :
                    nullptr);
            }
            for (const auto& i : p.hudLayouts)
            {
                i.second->setVisible(
                    p.hudActive &&
                    options.enabled &&
                    i.second->getChildren().size() > 0);
            }
        }
    }
}
