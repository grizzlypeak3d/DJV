// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <ftk/UI/IMouseWidget.h>

namespace djv
{
    namespace app
    {
        //! Shuttle widget.
        class ShuttleWidget : public ftk::IMouseWidget
        {
            FTK_NON_COPYABLE(ShuttleWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::string& iconPrefix,
                const std::shared_ptr<IWidget>& parent);

            ShuttleWidget();

        public:
            ~ShuttleWidget();

            static std::shared_ptr<ShuttleWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::string& iconPrefix,
                const std::shared_ptr<IWidget>& parent = nullptr);

            void setActiveCallback(const std::function<void(bool)>&);
            void setCallback(const std::function<void(int)>&);

            ftk::Size2I getSizeHint() const override;
            void sizeHintEvent(const ftk::SizeHintEvent&) override;
            void drawEvent(const ftk::Box2I&, const ftk::DrawEvent&) override;
            void mouseEnterEvent(ftk::MouseEnterEvent&) override;
            void mouseLeaveEvent() override;
            void mouseMoveEvent(ftk::MouseMoveEvent&) override;
            void mousePressEvent(ftk::MouseClickEvent&) override;
            void mouseReleaseEvent(ftk::MouseClickEvent&) override;

        private:
            FTK_PRIVATE();
        };
    }
}
