// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/UI/Export.h>
#include <djv/Models/SettingsModel.h>

#include <ftk/UI/IContainer.h>

namespace tl
{
    class Player;
}

namespace djv
{
    namespace models
    {
        class ColorModel;
        class FilesModel;
        class TimeUnitsModel;
        class ViewportModel;
    }

    namespace ui
    {
        //! Export widget.
        class DJV_UI_API_TYPE ExportWidget : public ftk::IContainer
        {
            FTK_NON_COPYABLE(ExportWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::FilesModel>&,
                const std::shared_ptr<models::ColorModel>&,
                const std::shared_ptr<models::ViewportModel>&,
                const std::shared_ptr<models::SettingsModel>&,
                const std::shared_ptr<models::TimeUnitsModel>&,
                const std::shared_ptr<IWidget>& parent);

            ExportWidget();

        public:
            DJV_UI_API virtual ~ExportWidget();

            DJV_UI_API static std::shared_ptr<ExportWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::FilesModel>&,
                const std::shared_ptr<models::ColorModel>&,
                const std::shared_ptr<models::ViewportModel>&,
                const std::shared_ptr<models::SettingsModel>&,
                const std::shared_ptr<models::TimeUnitsModel>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            //! Set the player.
            DJV_UI_API void setPlayer(const std::shared_ptr<tl::Player>&);

        private:
            std::vector<ftk::ImageInfo> _getInfos() const;
            ftk::Size2I _getDefaultSize() const;
            ftk::Size2I _getWidthSize(int width) const;
            ftk::Size2I _getExportSize(const models::ExportSettings&) const;
            void _sizeUpdate();
            void _widgetUpdate(const models::ExportSettings&);
            OTIO_NS::TimeRange _getExportRange(models::ExportFileType) const;
            void _export(models::ExportFileType);
            void _exportStart(models::ExportFileType);
            bool _exportFrame();
            void _exportAudio();

            FTK_PRIVATE();
        };
    }
}
