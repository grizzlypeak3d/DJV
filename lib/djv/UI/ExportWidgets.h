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
        class SettingsModel;
        class TimeUnitsModel;
    }

    namespace ui
    {

        //! Get an export file name.
        DJV_UI_API std::string getExportFileName(
            const models::ExportSettings&,
            models::ExportFileType,
            int64_t frame);

        //! Whether exporting the given range would overwrite anything that
        //! is already on disk.
        DJV_UI_API bool getExportExists(
            const models::ExportSettings&,
            models::ExportFileType,
            const OTIO_NS::TimeRange&);

        //! Base class for export widgets.
        class DJV_UI_API_TYPE IExportWidget : public ftk::IContainer
        {
            FTK_NON_COPYABLE(IExportWidget);

        protected:
            IExportWidget() = default;

        public:
            DJV_UI_API virtual ~IExportWidget();
        };

        //! Image export widget.
        class DJV_UI_API_TYPE ImageExportWidget : public IExportWidget
        {
            FTK_NON_COPYABLE(ImageExportWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::SettingsModel>&,
                const std::shared_ptr<IWidget>& parent);

            ImageExportWidget();

        public:
            DJV_UI_API virtual ~ImageExportWidget();

            DJV_UI_API static std::shared_ptr<ImageExportWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::SettingsModel>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            //! Set the player.
            DJV_UI_API void setPlayer(const std::shared_ptr<tl::Player>&);

            //! Set the export callback.
            DJV_UI_API void setExportCallback(const std::function<void(void)>&);

        private:
            void _infoUpdate();

            FTK_PRIVATE();
        };

        //! Image sequence export widget.
        class DJV_UI_API_TYPE SeqExportWidget : public IExportWidget
        {
            FTK_NON_COPYABLE(SeqExportWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::SettingsModel>&,
                const std::shared_ptr<models::TimeUnitsModel>&,
                const std::shared_ptr<IWidget>& parent);

            SeqExportWidget();

        public:
            DJV_UI_API virtual ~SeqExportWidget();

            DJV_UI_API static std::shared_ptr<SeqExportWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::SettingsModel>&,
                const std::shared_ptr<models::TimeUnitsModel>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            //! Set the player.
            DJV_UI_API void setPlayer(const std::shared_ptr<tl::Player>&);

            //! Set the export callback.
            DJV_UI_API void setExportCallback(const std::function<void(void)>&);

        private:
            void _infoUpdate();

            FTK_PRIVATE();
        };

        //! Movie export widget.
        class DJV_UI_API_TYPE MovieExportWidget : public IExportWidget
        {
            FTK_NON_COPYABLE(MovieExportWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::SettingsModel>&,
                const std::shared_ptr<models::TimeUnitsModel>&,
                const std::shared_ptr<IWidget>& parent);

            MovieExportWidget();

        public:
            DJV_UI_API virtual ~MovieExportWidget();

            DJV_UI_API static std::shared_ptr<MovieExportWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::SettingsModel>&,
                const std::shared_ptr<models::TimeUnitsModel>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            //! Set the player.
            DJV_UI_API void setPlayer(const std::shared_ptr<tl::Player>&);

            //! Set the export callback.
            DJV_UI_API void setExportCallback(const std::function<void(void)>&);

        private:
            void _infoUpdate();

            FTK_PRIVATE();
        };
    }
}
