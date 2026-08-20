// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/App/FilesTool.h>
#include <djv/Models/Export.h>

#include <tlRender/IO/IO.h>

#include <ftk/UI/IWidget.h>

namespace djv
{
    namespace ui
    {
        class DJV_API_TYPE FileThumbnail : public ftk::IWidget
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
            DJV_API virtual ~FileThumbnail();

            DJV_API static std::shared_ptr<FileThumbnail> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::FilesModelItem>&,
                const tl::IOOptions&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            DJV_API ftk::Size2I getSizeHint() const override;
            DJV_API void tickEvent(
                bool,
                bool,
                const ftk::TickEvent&) override;
            DJV_API void sizeHintEvent(const ftk::SizeHintEvent&) override;
            DJV_API void drawEvent(const ftk::Box2I&, const ftk::DrawEvent&) override;

        private:
            FTK_PRIVATE();
        };
    }
}
