// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/UI/Export.h>

#include <tlRender/UI/Viewport.h>

#include <ftk/UI/IContainer.h>

namespace ftk
{
    class Settings;
}

namespace djv
{
    namespace models
    {
        class ColorModel;
        class FilesModel;
        class SettingsModel;
        class ViewportModel;
    }

    namespace ui
    {
        //! Magnification level.
        enum class DJV_UI_API_TYPE MagnifyLevel
        {
            _2X,
            _4X,
            _8X,
            _16X,
            _32X,
            _64X,
            _128X,

            Count,
            First = _2X
        };
        FTK_ENUM(DJV_UI_API, MagnifyLevel);

        //! Get a magnification level.
        DJV_UI_API int getMagnifyLevel(MagnifyLevel);

        //! Magnify widget: a magnified view that follows another
        //! viewport's sample position.
        class DJV_UI_API_TYPE MagnifyWidget : public ftk::IContainer
        {
            FTK_NON_COPYABLE(MagnifyWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<ftk::Settings>&,
                const std::shared_ptr<tl::ui::Viewport>&,
                const std::shared_ptr<models::FilesModel>&,
                const std::shared_ptr<models::ColorModel>&,
                const std::shared_ptr<models::ViewportModel>&,
                const std::shared_ptr<models::SettingsModel>&,
                const std::shared_ptr<IWidget>& parent);

            MagnifyWidget();

        public:
            DJV_UI_API virtual ~MagnifyWidget();

            DJV_UI_API static std::shared_ptr<MagnifyWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<ftk::Settings>&,
                const std::shared_ptr<tl::ui::Viewport>&,
                const std::shared_ptr<models::FilesModel>&,
                const std::shared_ptr<models::ColorModel>&,
                const std::shared_ptr<models::ViewportModel>&,
                const std::shared_ptr<models::SettingsModel>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            //! Set the player.
            DJV_UI_API void setPlayer(const std::shared_ptr<tl::Player>&);

            DJV_UI_API void setGeometry(const ftk::Box2I&) override;

        private:
            void _widgetUpdate();
            void _videoUpdate();

            FTK_PRIVATE();
        };
    }
}
