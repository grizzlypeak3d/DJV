// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/UI/Export.h>

#include <tlRender/UI/Viewport.h>

namespace ftk
{
    class SysLogModel;
}

namespace djv
{
    namespace models
    {
        class ColorModel;
        class FilesModel;
        class SettingsModel;
        class TimeUnitsModel;
        class ViewportModel;
    }

    namespace ui
    {
        //! Viewport.
        class DJV_UI_API_TYPE Viewport : public tl::ui::Viewport
        {
            FTK_NON_COPYABLE(Viewport);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::FilesModel>&,
                const std::shared_ptr<models::ColorModel>&,
                const std::shared_ptr<models::ViewportModel>&,
                const std::shared_ptr<models::TimeUnitsModel>&,
                const std::shared_ptr<models::SettingsModel>&,
                const std::shared_ptr<ftk::SysLogModel>&,
                const std::shared_ptr<IWidget>& parent);

            Viewport();

        public:
            DJV_UI_API virtual ~Viewport();

            DJV_UI_API static std::shared_ptr<Viewport> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::FilesModel>&,
                const std::shared_ptr<models::ColorModel>&,
                const std::shared_ptr<models::ViewportModel>&,
                const std::shared_ptr<models::TimeUnitsModel>&,
                const std::shared_ptr<models::SettingsModel>&,
                const std::shared_ptr<ftk::SysLogModel>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            DJV_UI_API void setPlayer(const std::shared_ptr<tl::Player>&) override;

            //! Set whether the toast stands in for the status bar.
            DJV_UI_API void setToastActive(bool);

            //! Set whether the heads up display is shown at all. Separate from
            //! the display's own options so that turning it off for
            //! presentation does not forget what was being shown.
            DJV_UI_API void setHUDActive(bool);

            DJV_UI_API ftk::Size2I getSizeHint() const override;
            DJV_UI_API void setGeometry(const ftk::Box2I&) override;
            DJV_UI_API void mouseMoveEvent(ftk::MouseMoveEvent&) override;
            DJV_UI_API void mousePressEvent(ftk::MouseClickEvent&) override;
            DJV_UI_API void mouseReleaseEvent(ftk::MouseClickEvent&) override;

        private:
            void _videoUpdate();
            void _toastUpdate();
            void _compareUpdate();
            void _hudUpdate();
            void _hudLayout();

            FTK_PRIVATE();
        };
    }
}
