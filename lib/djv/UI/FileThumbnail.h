// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/UI/Export.h>
#include <djv/App/FilesTool.h>
#include <djv/Models/Export.h>

#include <tlRender/IO/IO.h>

#include <ftk/UI/IMouseWidget.h>

namespace djv
{
    namespace ui
    {
        //! What dragging a file carries: the item itself. The receiver
        //! looks the item up in the list it has, so a drag from a stale
        //! row cannot name a row that no longer exists.
        class DJV_UI_API_TYPE FileDragDropData : public ftk::IDragDropData
        {
        public:
            DJV_UI_API FileDragDropData(const std::shared_ptr<models::FilesModelItem>&);
            DJV_UI_API virtual ~FileDragDropData();

            DJV_UI_API const std::shared_ptr<models::FilesModelItem>& getItem() const;

        private:
            std::shared_ptr<models::FilesModelItem> _item;
        };

        class DJV_UI_API_TYPE FileThumbnail : public ftk::IMouseWidget
        {
            FTK_NON_COPYABLE(FileThumbnail);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::FilesModelItem>&,
                const tl::IOOptions&,
                const std::shared_ptr<IWidget>& parent);

            FileThumbnail();

        public:
            DJV_UI_API virtual ~FileThumbnail();

            DJV_UI_API static std::shared_ptr<FileThumbnail> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::FilesModelItem>&,
                const tl::IOOptions&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            //! Get the thumbnail image, e.g. for a drag cursor. Null until
            //! the thumbnail has loaded.
            DJV_UI_API const std::shared_ptr<ftk::Image>& getThumbnail() const;

            DJV_UI_API ftk::Size2I getSizeHint() const override;
            DJV_UI_API void tickEvent(
                bool,
                bool,
                const ftk::TickEvent&) override;
            DJV_UI_API void sizeHintEvent(const ftk::SizeHintEvent&) override;
            DJV_UI_API void drawEvent(const ftk::Box2I&, const ftk::DrawEvent&) override;

        private:
            FTK_PRIVATE();
        };
    }
}
