// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <ftk/UI/IDialog.h>
#include <ftk/Core/Path.h>

namespace djv
{
    namespace ui
    {
        //! Separate audio widget.
        class SeparateAudioWidget : public ftk::IMouseWidget
        {
            FTK_NON_COPYABLE(SeparateAudioWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<IWidget>& parent);

            SeparateAudioWidget();

        public:
            virtual ~SeparateAudioWidget();

            static std::shared_ptr<SeparateAudioWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            void setCallback(const std::function<void(
                const ftk::Path&,
                const ftk::Path&)>&);

            void setCancelCallback(const std::function<void(void)>&);

            //! Get the video path entry, which is the dialog's default
            //! focus.
            std::shared_ptr<ftk::IWidget> getFirstField() const;

            ftk::Size2I getSizeHint() const override;
            void setGeometry(const ftk::Box2I&) override;

        private:
            void _widgetUpdate();

            FTK_PRIVATE();
        };

        //! Separate audio dialog.
        class SeparateAudioDialog : public ftk::IDialog
        {
            FTK_NON_COPYABLE(SeparateAudioDialog);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<IWidget>& parent);

            SeparateAudioDialog();

        public:
            virtual ~SeparateAudioDialog();

            static std::shared_ptr<SeparateAudioDialog> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            //! Set the callback.
            void setCallback(const std::function<void(
                const ftk::Path&,
                const ftk::Path&)>&);

            std::shared_ptr<ftk::IWidget> getKeyFocus() const override;

        private:
            FTK_PRIVATE();
        };
    }
}
