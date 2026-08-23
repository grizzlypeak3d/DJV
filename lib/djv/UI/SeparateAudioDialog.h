// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/UI/Export.h>
#include <djv/Models/Export.h>

#include <ftk/UI/IDialog.h>
#include <ftk/Core/Path.h>

namespace djv
{
    namespace ui
    {
        //! Separate audio widget.
        class DJV_UI_API_TYPE SeparateAudioWidget : public ftk::IMouseWidget
        {
            FTK_NON_COPYABLE(SeparateAudioWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<IWidget>& parent);

            SeparateAudioWidget();

        public:
            DJV_UI_API virtual ~SeparateAudioWidget();

            DJV_UI_API static std::shared_ptr<SeparateAudioWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            DJV_UI_API void setCallback(const std::function<void(
                const ftk::Path&,
                const ftk::Path&)>&);

            DJV_UI_API void setCancelCallback(const std::function<void(void)>&);

            //! Get the video path entry, which is the dialog's default
            //! focus.
            DJV_UI_API std::shared_ptr<ftk::IWidget> getFirstField() const;

            DJV_UI_API ftk::Size2I getSizeHint() const override;
            DJV_UI_API void setGeometry(const ftk::Box2I&) override;

        private:
            void _widgetUpdate();

            FTK_PRIVATE();
        };

        //! Separate audio dialog.
        class DJV_UI_API_TYPE SeparateAudioDialog : public ftk::IDialog
        {
            FTK_NON_COPYABLE(SeparateAudioDialog);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<IWidget>& parent);

            SeparateAudioDialog();

        public:
            DJV_UI_API virtual ~SeparateAudioDialog();

            DJV_UI_API static std::shared_ptr<SeparateAudioDialog> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            //! Set the callback.
            DJV_UI_API void setCallback(const std::function<void(
                const ftk::Path&,
                const ftk::Path&)>&);

            DJV_UI_API std::shared_ptr<ftk::IWidget> getKeyFocus() const override;

        private:
            FTK_PRIVATE();
        };
    }
}
