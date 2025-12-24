// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djvApp/Tools/FilesToolPrivate.h>

#include <tlRender/UI/ThumbnailSystem.h>

#include <ftk/UI/DrawUtil.h>
#include <ftk/Core/Context.h>

#include <optional>

namespace djv
{
    namespace app
    {
        struct FileButton::Private
        {
            std::shared_ptr<FilesModelItem> item;

            struct SizeData
            {
                std::optional<float> displayScale;
                int margin = 0;
                int spacing = 0;
                int border = 0;
                ftk::FontInfo fontInfo;
                ftk::FontMetrics fontMetrics;
                ftk::Size2I textSize;
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

            struct DrawData
            {
                std::vector<std::shared_ptr<ftk::Glyph> > glyphs;
            };
            std::optional<DrawData> draw;
        };

        void FileButton::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<FilesModelItem>& item,
            const std::shared_ptr<IWidget>& parent)
        {
            IButton::_init(context, "djv::app::FileButton", parent);
            FTK_P();
            const std::string s = ftk::elide(item->path.getFileName());
            setText(s);
            setCheckable(true);
            setHStretch(ftk::Stretch::Expanding);
            setAcceptsKeyFocus(true);
            _buttonRole = ftk::ColorRole::None;
            p.item = item;
        }

        FileButton::FileButton() :
            _p(new Private)
        {}

        FileButton::~FileButton()
        {}

        std::shared_ptr<FileButton> FileButton::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<FilesModelItem>& item,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<FileButton>(new FileButton);
            out->_init(context, item, parent);
            return out;
        }

        void FileButton::tickEvent(
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

        void FileButton::sizeHintEvent(const ftk::SizeHintEvent& event)
        {
            IButton::sizeHintEvent(event);
            FTK_P();

            if (!p.size.displayScale.has_value() ||
                (p.size.displayScale.has_value() && p.size.displayScale.value() != event.displayScale))
            {
                p.size.displayScale = event.displayScale;
                p.size.margin = event.style->getSizeRole(ftk::SizeRole::MarginInside, event.displayScale);
                p.size.spacing = event.style->getSizeRole(ftk::SizeRole::SpacingSmall, event.displayScale);
                p.size.border = event.style->getSizeRole(ftk::SizeRole::Border, event.displayScale);
                p.size.fontInfo = event.style->getFontRole(_fontRole, event.displayScale);
                p.size.fontMetrics = event.fontSystem->getMetrics(p.size.fontInfo);
                p.size.textSize = event.fontSystem->getSize(_text, p.size.fontInfo);
                p.draw.reset();
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
                        reinterpret_cast<intptr_t>(p.item.get()),
                        p.item->path,
                        p.thumbnail.height);
                }
            }

            ftk::Size2I thumbnailSize;
            if (p.thumbnail.image)
            {
                const ftk::Size2I& size = p.thumbnail.image->getSize();
                thumbnailSize = ftk::Size2I(size.w * p.thumbnail.image->getInfo().pixelAspectRatio, size.h);
            }
            ftk::Size2I sizeHint;
            sizeHint.w =
                thumbnailSize.w +
                p.size.spacing +
                p.size.textSize.w + p.size.margin * 2 +
                p.size.margin * 2 +
                p.size.border * 4;
            sizeHint.h =
                std::max(p.size.fontMetrics.lineHeight, thumbnailSize.h) +
                p.size.margin * 2 +
                p.size.border * 4;
            setSizeHint(sizeHint);
        }

        void FileButton::clipEvent(const ftk::Box2I& clipRect, bool clipped)
        {
            IButton::clipEvent(clipRect, clipped);
            FTK_P();
            if (clipped)
            {
                p.draw.reset();
            }
        }

        void FileButton::drawEvent(
            const ftk::Box2I& drawRect,
            const ftk::DrawEvent& event)
        {
            IButton::drawEvent(drawRect, event);
            FTK_P();

            if (!p.draw.has_value())
            {
                p.draw = Private::DrawData();
            }

            const ftk::Box2I& g = getGeometry();
            const bool enabled = isEnabled();

            if (hasKeyFocus())
            {
                event.render->drawMesh(
                    ftk::border(g, p.size.border * 2),
                    event.style->getColorRole(ftk::ColorRole::KeyFocus));
            }

            const ftk::Box2I g2 = ftk::margin(g, -p.size.border * 2);
            if (isChecked())
            {
                event.render->drawRect(
                    g2,
                    event.style->getColorRole(ftk::ColorRole::Checked));
            }
            if (_isMousePressed() && ftk::contains(g, _getMousePos()))
            {
                event.render->drawRect(
                    g2,
                    event.style->getColorRole(ftk::ColorRole::Pressed));
            }
            else if (_isMouseInside())
            {
                event.render->drawRect(
                    g2,
                    event.style->getColorRole(ftk::ColorRole::Hover));
            }

            const ftk::Box2I g3 = ftk::margin(g2, -p.size.margin);
            int x = g3.min.x;
            if (p.thumbnail.image)
            {
                const ftk::Size2I& size = p.thumbnail.image->getSize();
                const ftk::Size2I thumbnailSize(size.w * p.thumbnail.image->getInfo().pixelAspectRatio, size.h);
                event.render->drawImage(
                    p.thumbnail.image,
                    ftk::Box2I(x, g3.y(), thumbnailSize.w, thumbnailSize.h));
                x += thumbnailSize.w + p.size.spacing;
            }

            if (!_text.empty())
            {
                if (p.draw->glyphs.empty())
                {
                    p.draw->glyphs = event.fontSystem->getGlyphs(_text, p.size.fontInfo);
                }
                const ftk::V2I pos(
                    x + p.size.margin,
                    g3.y() + g3.h() / 2 - p.size.textSize.h / 2);
                event.render->drawText(
                    p.draw->glyphs,
                    p.size.fontMetrics,
                    pos,
                    event.style->getColorRole(ftk::ColorRole::Text));
            }
        }

        void FileButton::keyPressEvent(ftk::KeyEvent& event)
        {
            FTK_P();
            if (0 == event.modifiers)
            {
                switch (event.key)
                {
                case ftk::Key::Return:
                    event.accept = true;
                    click();
                    break;
                case ftk::Key::Escape:
                    if (hasKeyFocus())
                    {
                        event.accept = true;
                        releaseKeyFocus();
                    }
                    break;
                default: break;
                }
            }
        }

        void FileButton::keyReleaseEvent(ftk::KeyEvent& event)
        {
            event.accept = true;
        }
    }
}
