// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/Models/SettingsModel.h>

#include <ftk/UI/IWidget.h>

namespace djv
{
    namespace app
    {
        class App;

        //! Get an export file name.
        std::string getExportFileName(
            const models::ExportSettings&,
            models::ExportFileType,
            double frame);

        //! Base class for export widgets.
        class IExportWidget : public ftk::IWidget
        {
            FTK_NON_COPYABLE(IExportWidget);

        protected:
            IExportWidget() = default;

        public:
            virtual ~IExportWidget();
        };

        //! Image export widget.
        class ImageExportWidget : public IExportWidget
        {
            FTK_NON_COPYABLE(ImageExportWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<IWidget>& parent);

            ImageExportWidget();

        public:
            virtual ~ImageExportWidget();

            static std::shared_ptr<ImageExportWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            //! Set the export callback.
            void setExportCallback(const std::function<void(void)>&);

            ftk::Size2I getSizeHint() const override;
            void setGeometry(const ftk::Box2I&) override;

        private:
            void _infoUpdate();

            FTK_PRIVATE();
        };

        //! Image sequence export widget.
        class SeqExportWidget : public IExportWidget
        {
            FTK_NON_COPYABLE(SeqExportWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<IWidget>& parent);

            SeqExportWidget();

        public:
            virtual ~SeqExportWidget();

            static std::shared_ptr<SeqExportWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            //! Set the export callback.
            void setExportCallback(const std::function<void(void)>&);

            ftk::Size2I getSizeHint() const override;
            void setGeometry(const ftk::Box2I&) override;

        private:
            void _infoUpdate();

            FTK_PRIVATE();
        };

        //! Movie export widget.
        class MovieExportWidget : public IExportWidget
        {
            FTK_NON_COPYABLE(MovieExportWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<IWidget>& parent);

            MovieExportWidget();

        public:
            virtual ~MovieExportWidget();

            static std::shared_ptr<MovieExportWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            //! Set the export callback.
            void setExportCallback(const std::function<void(void)>&);

            ftk::Size2I getSizeHint() const override;
            void setGeometry(const ftk::Box2I&) override;

        private:
            void _infoUpdate();

            FTK_PRIVATE();
        };
    }
}
