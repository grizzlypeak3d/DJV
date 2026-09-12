// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UI/FileThumbnail.h>

#include <tlRender/UI/ThumbnailSystem.h>

#include <ftk/UI/DrawUtil.h>
#include <ftk/Core/Context.h>

#include <algorithm>
#include <optional>

namespace djv
{
    namespace ui
    {
        FileDragDropData::FileDragDropData(
            const std::shared_ptr<models::FilesModelItem>& item) :
            _item(item)
        {}

        FileDragDropData::~FileDragDropData()
        {}

        const std::shared_ptr<models::FilesModelItem>& FileDragDropData::getItem() const
        {
            return _item;
        }

        struct FileThumbnail::Private
        {
            std::shared_ptr<models::FilesModelItem> item;
            tl::IOOptions ioOptions;

            struct SizeData
            {
                bool init = true;
                int margin = 0;
            };
            SizeData size;

            struct ThumbnailData
            {
                float scale = 1.F;
                int height = 40;
                tl::ui::ThumbnailRequest request;
                bool requestDone = false;
                tl::ui::InfoRequest infoRequest;
                bool infoDone = false;
                std::shared_ptr<ftk::Image> image;
            };
            ThumbnailData thumbnail;

            std::function<void(const tl::IOInfo&)> infoCallback;
        };

        void FileThumbnail::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::FilesModelItem>& item,
            const tl::IOOptions& ioOptions,
            const std::shared_ptr<IWidget>& parent)
        {
            IMouseWidget::_init(context, "djv::ui::FileThumbnail", parent);
            // The row owns the gestures: the whole file row is draggable
            // and clickable, so the thumbnail leaves the press alone -- and
            // the hover, which is what the button's click follows.
            _setMousePressEnabled(false);
            _setMouseHoverEnabled(false);
            FTK_P();
            p.item = item;
            p.ioOptions = ioOptions;
        }

        const std::shared_ptr<ftk::Image>& FileThumbnail::getThumbnail() const
        {
            return _p->thumbnail.image;
        }

        void FileThumbnail::setInfoCallback(
            const std::function<void(const tl::IOInfo&)>& value)
        {
            _p->infoCallback = value;
        }

        FileThumbnail::FileThumbnail() :
            _p(new Private)
        {}

        FileThumbnail::~FileThumbnail()
        {}

        std::shared_ptr<FileThumbnail> FileThumbnail::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::FilesModelItem>& item,
            const tl::IOOptions& ioOptions,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<FileThumbnail>(new FileThumbnail);
            out->_init(context, item, ioOptions, parent);
            return out;
        }

        ftk::Size2I FileThumbnail::getSizeHint() const
        {
            FTK_P();
            // A fixed slot rather than the image's own width: a portrait
            // thumbnail beside a widescreen one would start the next
            // column at a different place on every row.
            return ftk::Size2I(
                p.thumbnail.height * 16 / 9,
                p.thumbnail.height) + p.size.margin * 2;
        }

        void FileThumbnail::tickEvent(
            bool parentsVisible,
            bool parentsEnabled,
            const ftk::TickEvent& event)
        {
            IMouseWidget::tickEvent(parentsVisible, parentsEnabled, event);
            FTK_P();
            if (!isClipped())
            {
                _request();
            }
            if (p.thumbnail.request.future.valid() &&
                p.thumbnail.request.future.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
            {
                p.thumbnail.image = p.thumbnail.request.future.get();
                // Asked once, answered once, even when the answer is
                // nothing: a file with no picture in it -- audio only, or
                // one that cannot be read -- would otherwise be asked about
                // again on every tick.
                p.thumbnail.requestDone = true;
                setSizeUpdate();
                setDrawUpdate();
            }
            if (p.thumbnail.infoRequest.future.valid() &&
                p.thumbnail.infoRequest.future.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
            {
                const tl::IOInfo info = p.thumbnail.infoRequest.future.get();
                p.thumbnail.infoRequest = tl::ui::InfoRequest();
                p.thumbnail.infoDone = true;
                if (p.infoCallback)
                {
                    p.infoCallback(info);
                }
            }
        }

        void FileThumbnail::sizeHintEvent(const ftk::SizeHintEvent& event)
        {
            FTK_P();

            IMouseWidget::sizeHintEvent(event);
            if (p.size.init)
            {
                p.size.init = false;
                p.size.margin = event.style->getSizeRole(ftk::SizeRole::MarginInside, event.displayScale);
            }

            if (event.displayScale != p.thumbnail.scale)
            {
                // A different size is a different thumbnail; the next tick
                // asks for it, if the row is in view.
                p.thumbnail.scale = event.displayScale;
                p.thumbnail.height = 40 * event.displayScale;
                _cancelRequests();
                p.thumbnail.image.reset();
                p.thumbnail.requestDone = false;
            }
        }

        void FileThumbnail::clipEvent(const ftk::Box2I& clipRect, bool clipped)
        {
            IMouseWidget::clipEvent(clipRect, clipped);
            if (clipped)
            {
                // Scrolled out of view: what has not been answered yet is no
                // longer worth reading the file for. What did arrive is kept,
                // so scrolling back does not read it again.
                _cancelRequests();
            }
        }

        void FileThumbnail::_request()
        {
            FTK_P();
            auto context = getContext();
            if (!context || p.thumbnail.height <= 0)
            {
                return;
            }
            auto thumbnailSystem = context->getSystem<tl::ui::ThumbnailSystem>();
            if (!p.thumbnail.requestDone && !p.thumbnail.request.future.valid())
            {
                p.thumbnail.request = thumbnailSystem->getThumbnail(
                    p.item->path,
                    p.thumbnail.height,
                    std::nullopt,
                    p.ioOptions,
                    tl::ui::ThumbnailType::Timeline,
                    p.item->audioPath);
            }
            if (!p.thumbnail.infoDone && !p.thumbnail.infoRequest.future.valid())
            {
                p.thumbnail.infoRequest = thumbnailSystem->getInfo(
                    p.item->path,
                    p.ioOptions,
                    p.item->audioPath);
            }
        }

        void FileThumbnail::_cancelRequests()
        {
            FTK_P();
            std::vector<uint64_t> ids;
            if (p.thumbnail.request.future.valid())
            {
                ids.push_back(p.thumbnail.request.id);
                p.thumbnail.request = tl::ui::ThumbnailRequest();
            }
            if (p.thumbnail.infoRequest.future.valid())
            {
                ids.push_back(p.thumbnail.infoRequest.id);
                p.thumbnail.infoRequest = tl::ui::InfoRequest();
            }
            if (!ids.empty())
            {
                if (auto context = getContext())
                {
                    context->getSystem<tl::ui::ThumbnailSystem>()->
                        cancelRequests(ids);
                }
            }
        }

        void FileThumbnail::drawEvent(
            const ftk::Box2I& drawRect,
            const ftk::DrawEvent& event)
        {
            FTK_P();
            if (p.thumbnail.image)
            {
                const ftk::Box2I& g = getGeometry();
                const ftk::Size2I& size = p.thumbnail.image->getSize();
                const ftk::Size2I imageSize(
                    size.w * p.thumbnail.image->getInfo().pixelAspectRatio,
                    size.h);
                // As large as the image fits in the slot. A thumbnail
                // arrives at the height it was asked for but at whatever
                // width the file's aspect gives, which is wider than the
                // slot for anything wider than 16:9; drawn at that width it
                // would spill over the file name.
                ftk::Size2I thumbnailSize = imageSize;
                if (imageSize.w > 0 && imageSize.h > 0)
                {
                    const float scale = std::min(
                        (g.w() - p.size.margin * 2) / static_cast<float>(imageSize.w),
                        (g.h() - p.size.margin * 2) / static_cast<float>(imageSize.h));
                    thumbnailSize = ftk::Size2I(
                        imageSize.w * scale,
                        imageSize.h * scale);
                }
                ftk::ImageOptions imageOptions;
                imageOptions.cache = false;
                // Centered in the slot; see getSizeHint().
                event.render->drawImage(
                    p.thumbnail.image,
                    ftk::Box2I(
                        g.min.x + (g.w() - thumbnailSize.w) / 2,
                        g.min.y + (g.h() - thumbnailSize.h) / 2,
                        thumbnailSize.w,
                        thumbnailSize.h),
                    ftk::Color4F(1.F, 1.F, 1.F),
                    imageOptions);
            }
        }

    }
}
