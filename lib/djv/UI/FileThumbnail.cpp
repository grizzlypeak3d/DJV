// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UI/FileThumbnail.h>

#include <tlRender/UI/ThumbnailSystem.h>

#include <ftk/UI/DrawUtil.h>
#include <ftk/Core/Context.h>

#include <optional>

namespace djv
{
    namespace ui
    {
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
                bool init = true;
                float scale = 1.F;
                int height = 40;
                tl::ui::ThumbnailRequest request;
                std::shared_ptr<ftk::Image> image;
            };
            ThumbnailData thumbnail;
        };

        void FileThumbnail::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::FilesModelItem>& item,
            const tl::IOOptions& ioOptions,
            const std::shared_ptr<IWidget>& parent)
        {
            IWidget::_init(context, "djv::ui::FileThumbnail", parent);
            FTK_P();
            p.item = item;
            p.ioOptions = ioOptions;
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
            ftk::Size2I thumbnailSize;
            if (p.thumbnail.image)
            {
                const ftk::Size2I& size = p.thumbnail.image->getSize();
                thumbnailSize = ftk::Size2I(size.w * p.thumbnail.image->getInfo().pixelAspectRatio, size.h);
            }
            return thumbnailSize + p.size.margin * 2;
        }

        void FileThumbnail::tickEvent(
            bool parentsVisible,
            bool parentsEnabled,
            const ftk::TickEvent& event)
        {
            IWidget::tickEvent(parentsVisible, parentsEnabled, event);
            FTK_P();
            if (p.thumbnail.request.future.valid() &&
                p.thumbnail.request.future.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
            {
                p.thumbnail.image = p.thumbnail.request.future.get();
                setSizeUpdate();
                setDrawUpdate();
            }
        }

        void FileThumbnail::sizeHintEvent(const ftk::SizeHintEvent& event)
        {
            FTK_P();

            if (p.size.init)
            {
                p.size.init = false;
                p.size.margin = event.style->getSizeRole(ftk::SizeRole::MarginInside, event.displayScale);
            }

            if (event.displayScale != p.thumbnail.scale)
            {
                p.thumbnail.init = true;
                p.thumbnail.scale = event.displayScale;
                p.thumbnail.height = 40 * event.displayScale;
            }
            if (p.thumbnail.init)
            {
                p.thumbnail.init = false;
                if (auto context = getContext())
                {
                    auto thumbnailSystem = context->getSystem<tl::ui::ThumbnailSystem>();
                    p.thumbnail.request = thumbnailSystem->getThumbnail(
                        p.item->path,
                        p.thumbnail.height,
                        tl::invalidTime,
                        p.ioOptions);
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
                const ftk::Size2I thumbnailSize(size.w * p.thumbnail.image->getInfo().pixelAspectRatio, size.h);
                ftk::ImageOptions imageOptions;
                imageOptions.cache = false;
                event.render->drawImage(
                    p.thumbnail.image,
                    ftk::Box2I(
                        g.min.x + p.size.margin,
                        g.min.y + p.size.margin,
                        thumbnailSize.w,
                        thumbnailSize.h),
                    ftk::Color4F(1.F, 1.F, 1.F),
                    imageOptions);
            }
        }
    }
}
