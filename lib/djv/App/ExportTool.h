// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/App/Export.h>
#include <djv/App/IToolWidget.h>
#include <djv/Models/Export.h>

#include <djv/Models/SettingsModel.h>

namespace djv
{
    namespace app
    {
        //! Export tool.
        class DJV_APP_API_TYPE ExportTool : public IToolWidget
        {
            FTK_NON_COPYABLE(ExportTool);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<MainWindow>&,
                const std::shared_ptr<IWidget>& parent);

            ExportTool();

        public:
            DJV_APP_API virtual ~ExportTool();

            DJV_APP_API static std::shared_ptr<ExportTool> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<MainWindow>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

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
