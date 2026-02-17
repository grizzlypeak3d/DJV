// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/App/FilesTool.h>

#include <ftk/UI/IButton.h>

namespace djv
{
    namespace ui
    {
        class FileButton : public ftk::IButton
        {
            FTK_NON_COPYABLE(FileButton);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::FilesModelItem>&,
                const std::shared_ptr<IWidget>& parent);

            FileButton();

        public:
            virtual ~FileButton();

            static std::shared_ptr<FileButton> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::FilesModelItem>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            ftk::Size2I getSizeHint() const override;
            void tickEvent(
                bool,
                bool,
                const ftk::TickEvent&) override;
            void sizeHintEvent(const ftk::SizeHintEvent&) override;
            void clipEvent(const ftk::Box2I&, bool) override;
            void drawEvent(const ftk::Box2I&, const ftk::DrawEvent&) override;
            void keyPressEvent(ftk::KeyEvent&) override;
            void keyReleaseEvent(ftk::KeyEvent&) override;

        private:
            FTK_PRIVATE();
        };
    }
}
