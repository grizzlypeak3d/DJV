// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/Viewport.h>

#include <djv/App/App.h>
#include <djv/Models/AnnotationsModel.h>
#include <djv/Models/ColorModel.h>
#include <djv/Models/DrawModel.h>
#include <djv/Models/FilesModel.h>
#include <djv/Models/SettingsModel.h>
#include <djv/Models/TimeUnitsModel.h>
#include <djv/Models/ViewportModel.h>

#include <tlRender/Timeline/Util.h>

#include <ftk/UI/ColorSwatch.h>
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
    namespace app
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
            std::weak_ptr<App> app;
            models::HUDOptions hudOptions;
            ftk::Path path;
            tl::IOInfo ioInfo;
            std::optional<OTIO_NS::RationalTime> currentTime;
            double fps = 0.0;
            size_t droppedFrames = 0;
            size_t videoFramesSize = 0;
            std::vector<tl::VideoFrame> videoFrames;
            ftk::ImageOptions imageOptions;
            tl::DisplayOptions displayOptions;
            tl::PlayerCacheInfo cacheInfo;
            double viewZoom = 0.0;
            models::MouseActionBinding pickBinding =
                models::MouseActionBinding(ftk::MouseButton::Left);
            models::MouseActionBinding frameShuttleBinding =
                models::MouseActionBinding(ftk::MouseButton::Left, ftk::KeyModifier::Shift);
            float frameShuttleScale = 1.F;
            std::shared_ptr<ftk::Observable<ftk::V2I> > pick;
            std::shared_ptr<ftk::Observable<ftk::V2I> > samplePos;
            std::shared_ptr<ftk::Observable<ftk::Color4F> > colorSample;

            std::shared_ptr<ftk::Label> fileNameLabel;
            std::shared_ptr<ftk::Label> cacheLabel;
            std::shared_ptr<ftk::Label> timeLabel;
            std::shared_ptr<ftk::Label> viewZoomLabel;
            std::shared_ptr<ftk::ColorSwatch> colorPickerSwatch;
            std::shared_ptr<ftk::Label> colorPickerLabel;
            std::shared_ptr<ftk::Label> infoLabel;
            std::shared_ptr<ftk::Label> renderLabel;
            std::map<models::HUDItem, std::shared_ptr<ftk::IWidget> > hudWidgets;
            bool toastActive = false;
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
            std::shared_ptr<ftk::Observer<tl::CompareOptions> > compareOptionsObserver;
            std::shared_ptr<ftk::Observer<tl::OCIOOptions> > ocioOptionsObserver;
            std::shared_ptr<ftk::Observer<tl::LUTOptions> > lutOptionsObserver;
            std::shared_ptr<ftk::Observer<ftk::ImageOptions> > imageOptionsObserver;
            std::shared_ptr<ftk::Observer<tl::DisplayOptions> > displayOptionsObserver;
            std::shared_ptr<ftk::Observer<tl::BackgroundOptions> > bgOptionsObserver;
            std::shared_ptr<ftk::Observer<tl::ForegroundOptions> > fgOptionsObserver;
            std::shared_ptr<ftk::Observer<ftk::gl::TextureType> > colorBufferObserver;
            std::shared_ptr<ftk::Observer<double> > viewZoomObserver;
            std::shared_ptr<ftk::ListObserver<std::string> > messagesObserver;
            std::shared_ptr<ftk::Observer<models::HUDOptions> > hudOptionsObserver;
            std::shared_ptr<ftk::Observer<tl::TimeUnits> > timeUnitsObserver;
            std::shared_ptr<ftk::Observer<models::MouseSettings> > mouseSettingsObserver;
            std::shared_ptr<ftk::ListObserver<models::ReviewAnnotation> > annotationsObserver;

            enum class MouseMode
            {
                None,
                Shuttle,
                Picker,
                Draw,
                Erase
            };
            struct MouseData
            {
                MouseMode mode = MouseMode::None;
                // Where the shuttle started, unset until it is grabbed.
                std::optional<OTIO_NS::RationalTime> shuttleStart;
            };
            MouseData mouse;

            //! The stroke being drawn, in the image pixels of its source.
            models::ReviewStroke stroke;
            int strokeSource = -1;
        };

        void Viewport::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            tl::ui::Viewport::_init(context, parent);
            FTK_P();

            p.app = app;

            p.pick = ftk::Observable<ftk::V2I>::create();
            p.samplePos = ftk::Observable<ftk::V2I>::create();
            p.colorSample = ftk::Observable<ftk::Color4F>::create();

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

            p.compareOptionsObserver = ftk::Observer<tl::CompareOptions>::create(
                app->getFilesModel()->observeCompareOptions(),
                [this](const tl::CompareOptions& value)
                {
                    setCompareOptions(value);
                });

            p.ocioOptionsObserver = ftk::Observer<tl::OCIOOptions>::create(
                app->getColorModel()->observeOCIOOptions(),
                [this](const tl::OCIOOptions& value)
                {
                   setOCIOOptions(value);
                });

            p.lutOptionsObserver = ftk::Observer<tl::LUTOptions>::create(
                app->getColorModel()->observeLUTOptions(),
                [this](const tl::LUTOptions& value)
                {
                   setLUTOptions(value);
                });

            p.imageOptionsObserver = ftk::Observer<ftk::ImageOptions>::create(
                app->getViewportModel()->observeImageOptions(),
                [this](const ftk::ImageOptions& value)
                {
                    _p->imageOptions = value;
                    _videoUpdate();
                });

            p.displayOptionsObserver = ftk::Observer<tl::DisplayOptions>::create(
                app->getViewportModel()->observeDisplayOptions(),
                [this](const tl::DisplayOptions& value)
                {
                    _p->displayOptions = value;
                    _videoUpdate();
                });

            p.bgOptionsObserver = ftk::Observer<tl::BackgroundOptions>::create(
                app->getViewportModel()->observeBackgroundOptions(),
                [this](const tl::BackgroundOptions& value)
                {
                    setBackgroundOptions(value);
                });

            p.fgOptionsObserver = ftk::Observer<tl::ForegroundOptions>::create(
                app->getViewportModel()->observeForegroundOptions(),
                [this](const tl::ForegroundOptions& value)
                {
                    setForegroundOptions(value);
                });

            p.colorBufferObserver = ftk::Observer<ftk::gl::TextureType>::create(
                app->getViewportModel()->observeColorBuffer(),
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

            p.messagesObserver = ftk::ListObserver<std::string>::create(
                app->getSysLogModel()->observeMessages(),
                [this](const std::vector<std::string>& value)
                {
                    FTK_P();
                    // A burst replaces rather than accumulates, matching the
                    // status bar: a sequence with many unreadable frames would
                    // otherwise queue a message per frame.
                    p.toastLabel->setText(!value.empty() ?
                        ftk::elide(value.back(), toastTextLength) :
                        std::string());
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
                app->getViewportModel()->observeHUDOptions(),
                [this](const models::HUDOptions& value)
                {
                    _p->hudOptions = value;
                    _hudUpdate();
                    _hudLayout();
                });

            p.timeUnitsObserver = ftk::Observer<tl::TimeUnits>::create(
                app->getTimeUnitsModel()->observeTimeUnits(),
                [this](tl::TimeUnits value)
                {
                    _hudUpdate();
                });

            // The overlay is drawn straight from the model, so any change to it
            // has to ask for a redraw. Drawing does that from the mouse events,
            // but undo, redo and "Clear Frame" only touch the model: without
            // this the frame keeps showing stale strokes until some unrelated
            // event happens to repaint the window.
            p.annotationsObserver = ftk::ListObserver<models::ReviewAnnotation>::create(
                app->getAnnotationsModel()->observeAnnotations(),
                [this](const std::vector<models::ReviewAnnotation>&)
                {
                    setDrawUpdate();
                });

            p.mouseSettingsObserver = ftk::Observer<models::MouseSettings>::create(
                app->getSettingsModel()->observeMouse(),
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
                    p.pickBinding = i != value.bindings.end() ? i->second : models::MouseActionBinding();
                    i = value.bindings.find(models::MouseAction::FrameShuttle);
                    p.frameShuttleBinding = i != value.bindings.end() ? i->second : models::MouseActionBinding();
                    p.frameShuttleScale = value.frameShuttleScale;
                });
        }

        Viewport::Viewport() :
            _p(new Private)
        {}

        Viewport::~Viewport()
        {}

        std::shared_ptr<Viewport> Viewport::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<Viewport>(new Viewport);
            out->_init(context, app, parent);
            return out;
        }

        std::shared_ptr<ftk::IObservable<ftk::V2I> > Viewport::observePick() const
        {
            return _p->pick;
        }

        std::shared_ptr<ftk::IObservable<ftk::V2I> > Viewport::observeSamplePos() const
        {
            return _p->samplePos;
        }

        std::shared_ptr<ftk::IObservable<ftk::Color4F> > Viewport::observeColorSample() const
        {
            return _p->colorSample;
        }

        bool Viewport::_getSourceBox(ftk::Box2I& box, ftk::Size2I& size) const
        {
            // With OTIO spatial coordinates the render space is the timeline
            // canvas rather than the media, so the box the image is drawn into
            // is needed to relate the two. Returns false without spatial
            // coordinates, where render space is already the image.
            if (auto player = getPlayer())
            {
                const auto& videoFrame = player->getCurrentVideo();
                if (!videoFrame.empty() && videoFrame.front().canvasSize.isValid())
                {
                    const auto& displayOptions = getDisplayOptions();
                    const tl::AspectRatioOptions aspectRatio =
                        !displayOptions.empty() ?
                        displayOptions.front().aspectRatio :
                        tl::AspectRatioOptions();
                    for (const auto& layer : videoFrame.front().layers)
                    {
                        const auto& image = layer.image ? layer.image : layer.imageB;
                        const auto& bounds = layer.image ? layer.bounds : layer.boundsB;
                        if (image && bounds.has_value())
                        {
                            // The image is fitted into its canvas box, so use
                            // the same box the renderer draws into.
                            box = tl::getBox(
                                ftk::Box2I(
                                    ftk::V2I(
                                        std::lround(bounds.value().min.x),
                                        std::lround(bounds.value().min.y)),
                                    ftk::V2I(
                                        std::lround(bounds.value().max.x),
                                        std::lround(bounds.value().max.y))),
                                image->getInfo(),
                                aspectRatio);
                            size = image->getInfo().size;
                            return box.w() > 0 && box.h() > 0 && size.isValid();
                        }
                    }
                }
            }
            return false;
        }

        ftk::V2I Viewport::_toSourcePixel(const ftk::V2I& renderPos) const
        {
            ftk::Box2I box;
            ftk::Size2I size;
            ftk::V2I out = renderPos;
            if (_getSourceBox(box, size))
            {
                out = ftk::V2I(
                    std::lround(
                        (renderPos.x - box.min.x) /
                        static_cast<double>(box.w()) * size.w),
                    std::lround(
                        (renderPos.y - box.min.y) /
                        static_cast<double>(box.h()) * size.h));
            }
            return out;
        }

        ftk::V2I Viewport::_fromSourcePixel(const ftk::V2I& sourcePos) const
        {
            ftk::Box2I box;
            ftk::Size2I size;
            ftk::V2I out = sourcePos;
            if (_getSourceBox(box, size))
            {
                out = ftk::V2I(
                    std::lround(
                        box.min.x + sourcePos.x /
                        static_cast<double>(size.w) * box.w()),
                    std::lround(
                        box.min.y + sourcePos.y /
                        static_cast<double>(size.h) * box.h()));
            }
            return out;
        }

        void Viewport::pick(const ftk::V2I& imagePos)
        {
            FTK_P();
            // Image pixel -> widget-local position, inverting the pick math used
            // by the mouse handlers (image = (widget - viewPos) / zoom). Then
            // sample exactly as a pick mouse action would. The incoming
            // position is a source pixel, so it goes through the canvas first.
            const ftk::V2I renderPos = _fromSourcePixel(imagePos);
            const auto viewPos = getViewPos();
            const double zoom = getZoom();
            const ftk::V2I pos(
                static_cast<int>(viewPos.x + renderPos.x * zoom),
                static_cast<int>(viewPos.y + renderPos.y * zoom));
            p.samplePos->setIfChanged(pos);
            p.colorSample->setIfChanged(getColorSample(pos));
            p.pick->setIfChanged(imagePos);
            _hudUpdate();
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
                        _p->videoFramesSize = value.size();
                        // Kept for the annotation hit test and overlay, which
                        // need the image sizes to map screen to image pixels.
                        _p->videoFrames = value;
                        _videoUpdate();
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
            case Private::MouseMode::Picker:
                if (auto app = p.app.lock())
                {
                    const ftk::Box2I& g = getGeometry();
                    const ftk::V2I pos = event.pos - g.min;
                    if (p.samplePos->setIfChanged(pos))
                    {
                        p.colorSample->setIfChanged(getColorSample(pos));
                        p.pick->setIfChanged(_toSourcePixel((pos - getViewPos()) / getZoom()));
                        _hudUpdate();
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

            // Drawing owns the plain left button while it is enabled, which is
            // why it is an explicit mode: it displaces the frame shuttle.
            auto app = p.app.lock();
            if (app &&
                app->getDrawModel()->isEnabled() &&
                ftk::MouseButton::Left == event.button &&
                0 == event.modifiers)
            {
                // Claim the button, as the picker and the shuttle do below.
                // Without this the release never reaches us: the stroke is
                // never committed, keeps growing on every move, and the undo
                // that follows has to copy all of it.
                event.accept = true;
                takeKeyFocus();
                const ftk::Box2I& g = getGeometry();
                const ftk::V2I pos = event.pos - g.min;
                if (models::DrawTool::Eraser == app->getDrawModel()->getTool())
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

            if (p.pickBinding.button == event.button &&
                ftk::checkKeyModifier(p.pickBinding.modifier, event.modifiers))
            {
                // The base class only claims the buttons bound to its own
                // actions, so claim ours here; an unbound button is left
                // free to open a context menu.
                event.accept = true;
                takeKeyFocus();
                p.mouse.mode = Private::MouseMode::Picker;
                const ftk::Box2I& g = getGeometry();
                const ftk::V2I pos = event.pos - g.min;
                if (p.samplePos->setIfChanged(pos))
                {
                    p.colorSample->setIfChanged(getColorSample(pos));
                    p.pick->setIfChanged(_toSourcePixel((pos - getViewPos()) / getZoom()));
                    _hudUpdate();
                }
            }
            else if (p.frameShuttleBinding.button == event.button &&
                ftk::checkKeyModifier(p.frameShuttleBinding.modifier, event.modifiers))
            {
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
            const float renderX = box.min.x + imagePos.x * box.w() / static_cast<float>(imageSize.w);
            const float renderY = box.min.y + imagePos.y * box.h() / static_cast<float>(imageSize.h);
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
            if (auto app = p.app.lock())
            {
                auto drawModel = app->getDrawModel();
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
                if (auto app = p.app.lock())
                {
                    const auto& active = app->getFilesModel()->getActive();
                    if (p.strokeSource < static_cast<int>(active.size()))
                    {
                        app->getAnnotationsModel()->addStroke(
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
            if (auto app = p.app.lock())
            {
                const auto& active = app->getFilesModel()->getActive();
                if (hit.index < static_cast<int>(active.size()))
                {
                    // The eraser removes whole strokes it touches; its reach
                    // follows the tool size.
                    app->getAnnotationsModel()->eraseStrokes(
                        active[hit.index]->id,
                        *p.currentTime,
                        hit.pos,
                        app->getDrawModel()->getSize());
                }
            }
        }

        void Viewport::drawEvent(const ftk::Box2I& drawRect, const ftk::DrawEvent& event)
        {
            tl::ui::Viewport::drawEvent(drawRect, event);
            FTK_P();

            auto app = p.app.lock();
            if (!app)
            {
                return;
            }
            const ftk::Box2I& g = getGeometry();
            const double zoom = getZoom();
            const auto& active = app->getFilesModel()->getActive();

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
            const auto& annotations = app->getAnnotationsModel()->getAnnotations();
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
                if (index < 0)
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
            }
            setImageOptions(imageOptionsList);
            setDisplayOptions(displayOptionsList);
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
            if (auto app = p.app.lock())
            {
                auto timeUnitsModel = app->getTimeUnitsModel();
                s = timeUnitsModel->getLabel(p.currentTime);
            }
            p.timeLabel->setText(ftk::Format("Time: {0}, {1} FPS, {2} dropped").
                arg(s).
                arg(p.fps, 2, 6).
                arg(static_cast<int>(p.droppedFrames), 3));
            ftk::setScreenshotTag(p.timeLabel, "View.HUD.Time");

            p.viewZoomLabel->setText(ftk::Format("Zoom: {0}").
                arg(p.viewZoom, 2, 6));
            ftk::setScreenshotTag(p.viewZoomLabel, "View.HUD.ViewZoom");

            const auto& colorSample = p.colorSample->get();
            p.colorPickerSwatch->setColor(colorSample);
            const auto& pick = p.pick->get();
            p.colorPickerLabel->setText(
                ftk::Format("Color: {0} {1} {2} {3}, Pixel: {4}, {5}").
                arg(colorSample.r, 2).
                arg(colorSample.g, 2).
                arg(colorSample.b, 2).
                arg(colorSample.a, 2).
                arg(pick.x, 4).
                arg(pick.y, 4));
            ftk::setScreenshotTag(p.colorPickerLabel, "View.HUD.ColorPicker");
            ftk::setScreenshotTag(p.colorPickerSwatch, "View.HUD.ColorPickerSwatch");

            p.cacheLabel->setText(
                ftk::Format("Cache: {0}% V, {1}% A").
                arg(static_cast<int>(p.cacheInfo.videoPercentage), 3).
                arg(static_cast<int>(p.cacheInfo.audioPercentage), 3));
            ftk::setScreenshotTag(p.cacheLabel, "View.HUD.Cache");
        }

        void Viewport::_toastUpdate()
        {
            FTK_P();
            p.toastLabel->setVisible(
                p.toastActive && !p.toastLabel->getText().empty());
        }

        void Viewport::_hudLayout()
        {
            FTK_P();
            auto app = p.app.lock();
            const auto options = app->getViewportModel()->getHUDOptions();
            for (const auto& i : options.items)
            {
                p.hudWidgets[i.first]->setParent(i.second != models::HUDPos::None ?
                    p.hudLayouts[i.second] :
                    nullptr);
            }
            for (const auto& i : p.hudLayouts)
            {
                i.second->setVisible(
                    options.enabled && i.second->getChildren().size() > 0);
            }
        }
    }
}
