// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UI/TimelinePreview.h>

#include <tlRender/Timeline/IRender.h>

#include <ftk/UI/DrawUtil.h>
#include <ftk/UI/Label.h>
#include <ftk/UI/ScreenshotTag.h>

namespace djv
{
    namespace ui
    {
        struct TimelinePreview::Private
        {
            std::shared_ptr<tl::Player> player;
            std::shared_ptr<tl::ITimeUnitsModel> timeUnitsModel;
            ftk::V2I pos;
            std::optional<OTIO_NS::RationalTime> time;
            tl::VideoRequest request;
            tl::VideoFrame frame;
            ftk::ImageOptions imageOptions;
            tl::DisplayOptions displayOptions;
            tl::OCIOOptions ocioOptions;
            std::function<std::string(const std::string&, const ftk::ImageTags&)> ocioInputResolver;
            tl::LUTOptions lutOptions;
            std::shared_ptr<ftk::Label> label;

            struct SizeData
            {
                bool init = true;
                float displayScale = 0.F;
                int margin = 0;
                int spacing = 0;
                int border = 0;
                int handle = 0;
                //! The height of the frame; its width follows the video.
                int height = 0;
            };
            SizeData size;

            struct DrawData
            {
                ftk::Box2I g;
                ftk::Box2I image;
                ftk::TriMesh2F border;
            };
            std::optional<DrawData> draw;
        };

        void TimelinePreview::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<tl::Player>& player,
            const std::shared_ptr<tl::ITimeUnitsModel>& timeUnitsModel,
            const std::shared_ptr<IWidget>& window)
        {
            IPopup::_init(context, "djv::ui::TimelinePreview", nullptr);
            FTK_P();
            p.player = player;
            p.timeUnitsModel = timeUnitsModel;
            p.label = ftk::Label::create(context, shared_from_this());
            p.label->setTextRole(ftk::ColorRole::TooltipText);
            p.label->setHAlign(ftk::HAlign::Center);
            ftk::setScreenshotTag(shared_from_this(), "Timeline.Preview");
            ftk::setScreenshotTag(p.label, "Timeline.PreviewTime");
            setParent(window);
        }

        TimelinePreview::TimelinePreview() :
            _p(new Private)
        {}

        TimelinePreview::~TimelinePreview()
        {
            FTK_P();
            if (p.request.future.valid() && p.player)
            {
                p.player->getTimeline()->cancelRequests({ p.request.id });
            }
        }

        std::shared_ptr<TimelinePreview> TimelinePreview::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<tl::Player>& player,
            const std::shared_ptr<tl::ITimeUnitsModel>& timeUnitsModel,
            const std::shared_ptr<IWidget>& window)
        {
            auto out = std::shared_ptr<TimelinePreview>(new TimelinePreview);
            out->_init(context, player, timeUnitsModel, window);
            return out;
        }

        void TimelinePreview::setPos(const ftk::V2I& value)
        {
            FTK_P();
            if (value == p.pos)
                return;
            p.pos = value;
            setSizeUpdate();
            setDrawUpdate();
        }

        void TimelinePreview::setTime(const OTIO_NS::RationalTime& value)
        {
            FTK_P();
            if (p.time.has_value() && value == p.time.value())
                return;
            p.time = value;
            // One request at a time: a cursor moving along the timeline
            // asks for a frame per position, and the ones it has already
            // moved past are not wanted.
            if (p.request.future.valid())
            {
                p.player->getTimeline()->cancelRequests({ p.request.id });
            }
            p.request = p.player->getTimeline()->getVideo(value);
            p.label->setText(p.timeUnitsModel->getLabel(value));
            setDrawUpdate();
        }

        void TimelinePreview::setImageOptions(const ftk::ImageOptions& value)
        {
            FTK_P();
            if (value == p.imageOptions)
                return;
            p.imageOptions = value;
            setDrawUpdate();
        }

        void TimelinePreview::setDisplayOptions(const tl::DisplayOptions& value)
        {
            FTK_P();
            if (value == p.displayOptions)
                return;
            p.displayOptions = value;
            setDrawUpdate();
        }

        void TimelinePreview::setOCIOOptions(const tl::OCIOOptions& value)
        {
            FTK_P();
            if (value == p.ocioOptions)
                return;
            p.ocioOptions = value;
            setDrawUpdate();
        }

        void TimelinePreview::setOCIOInputResolver(
            const std::function<std::string(const std::string&, const ftk::ImageTags&)>& value)
        {
            _p->ocioInputResolver = value;
        }

        void TimelinePreview::setLUTOptions(const tl::LUTOptions& value)
        {
            FTK_P();
            if (value == p.lutOptions)
                return;
            p.lutOptions = value;
            setDrawUpdate();
        }

        void TimelinePreview::close()
        {
            setParent(nullptr);
        }

        void TimelinePreview::setGeometry(const ftk::Box2I& value)
        {
            IPopup::setGeometry(value);
            FTK_P();

            // The frame's width follows the video's aspect ratio. A file
            // with no video -- an audio file -- has no frame to show, so the
            // preview is the time by itself rather than an empty box above
            // it.
            const auto& ioInfo = p.player->getIOInfo();
            ftk::Size2I imageSize;
            if (!ioInfo.video.empty())
            {
                imageSize = ftk::Size2I(
                    p.size.height * ftk::aspectRatio(ioInfo.video.front().size),
                    p.size.height);
            }
            // No frame, nothing to separate the time from.
            const int spacing = imageSize.h > 0 ? p.size.spacing : 0;
            const ftk::Size2I labelSize = p.label->getSizeHint();
            const ftk::Size2I size(
                std::max(imageSize.w, labelSize.w) + p.size.margin * 2,
                imageSize.h + spacing + labelSize.h + p.size.margin * 2);

            // Above the cursor and centered on it, kept inside the window.
            ftk::Box2I g(
                p.pos.x - size.w / 2,
                p.pos.y - p.size.handle - size.h,
                size.w,
                size.h);
            if (g.max.x > value.max.x)
            {
                const int diff = g.max.x - value.max.x;
                g.min.x -= diff;
                g.max.x -= diff;
            }
            if (g.min.x < value.min.x)
            {
                g.max.x += value.min.x - g.min.x;
                g.min.x = value.min.x;
            }
            if (g.min.y < value.min.y)
            {
                g.max.y += value.min.y - g.min.y;
                g.min.y = value.min.y;
            }

            const ftk::Box2I image(
                g.min.x + (g.w() - imageSize.w) / 2,
                g.min.y + p.size.margin,
                imageSize.w,
                imageSize.h);
            p.label->setGeometry(ftk::Box2I(
                g.min.x + p.size.margin,
                image.max.y + 1 + spacing,
                g.w() - p.size.margin * 2,
                labelSize.h));

            if (!p.draw.has_value() || g != p.draw->g)
            {
                p.draw = Private::DrawData();
                p.draw->g = g;
                p.draw->image = image;
                p.draw->border = ftk::border(g, p.size.border);
            }
        }

        void TimelinePreview::sizeHintEvent(const ftk::SizeHintEvent& event)
        {
            IPopup::sizeHintEvent(event);
            FTK_P();
            if (p.size.init || event.displayScale != p.size.displayScale)
            {
                p.size.init = false;
                p.size.displayScale = event.displayScale;
                p.size.margin = event.style->getSizeRole(ftk::SizeRole::MarginSmall, event.displayScale);
                p.size.spacing = event.style->getSizeRole(ftk::SizeRole::SpacingSmall, event.displayScale);
                p.size.border = event.style->getSizeRole(ftk::SizeRole::Border, event.displayScale);
                p.size.handle = event.style->getSizeRole(ftk::SizeRole::Handle, event.displayScale);
                p.size.height = 180 * event.displayScale;
                p.draw.reset();
            }
        }

        void TimelinePreview::tickEvent(
            bool parentsVisible,
            bool parentsEnabled,
            const ftk::TickEvent& event)
        {
            IPopup::tickEvent(parentsVisible, parentsEnabled, event);
            FTK_P();
            if (p.request.future.valid() &&
                p.request.future.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
            {
                p.frame = p.request.future.get();
                setDrawUpdate();
            }
        }

        void TimelinePreview::drawEvent(const ftk::Box2I& drawRect, const ftk::DrawEvent& event)
        {
            IPopup::drawEvent(drawRect, event);
            FTK_P();
            if (!p.draw.has_value())
                return;

            event.render->drawMesh(
                p.draw->border,
                event.style->getColorRole(ftk::ColorRole::Border));
            event.render->drawRect(
                ftk::margin(p.draw->g, -p.size.border),
                event.style->getColorRole(ftk::ColorRole::TooltipWindow));

            // Drawn the way the view draws its buffer, with the same render
            // state. The last frame that arrived stays up until the next one
            // does: a cursor moving along the timeline would otherwise show
            // an empty box between every two frames.
            if (auto render = std::dynamic_pointer_cast<tl::IRender>(event.render);
                render && !p.frame.layers.empty())
            {
                render->setOCIOOptions(p.ocioOptions);
                render->setOCIOInputResolver(p.ocioInputResolver);
                render->setLUTOptions(p.lutOptions);
                render->drawVideo(
                    { p.frame },
                    { p.draw->image },
                    { p.imageOptions },
                    { p.displayOptions },
                    tl::CompareOptions());
            }
        }
    }
}
