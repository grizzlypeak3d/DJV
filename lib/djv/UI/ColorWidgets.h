// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/Models/Export.h>
#include <djv/Models/OCIOModel.h>

#include <ftk/UI/IContainer.h>
#include <ftk/UI/IWidget.h>

namespace ftk
{
    class CheckBox;
    class Settings;
}

namespace djv
{
    namespace models
    {
        class ColorModel;
        class ViewportModel;
    }

    namespace ui
    {
        class DJV_API_TYPE OCIOWidget : public ftk::IContainer
        {
            FTK_NON_COPYABLE(OCIOWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::ColorModel>&,
                const std::shared_ptr<IWidget>& parent);

            OCIOWidget();

        public:
            DJV_API virtual ~OCIOWidget();

            DJV_API static std::shared_ptr<OCIOWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::ColorModel>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            DJV_API std::shared_ptr<ftk::CheckBox> getEnabledCheckBox() const;

        private:
            void _extUpdate();

            FTK_PRIVATE();
        };

        class DJV_API_TYPE LUTWidget : public ftk::IContainer
        {
            FTK_NON_COPYABLE(LUTWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::ColorModel>&,
                const std::shared_ptr<IWidget>& parent);

            LUTWidget();

        public:
            DJV_API virtual ~LUTWidget();

            DJV_API static std::shared_ptr<LUTWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::ColorModel>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            DJV_API std::shared_ptr<ftk::CheckBox> getEnabledCheckBox() const;

        private:
            FTK_PRIVATE();
        };

        class DJV_API_TYPE ColorWidget : public ftk::IContainer
        {
            FTK_NON_COPYABLE(ColorWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::ViewportModel>&,
                const std::shared_ptr<IWidget>& parent);

            ColorWidget();

        public:
            DJV_API virtual ~ColorWidget();

            DJV_API static std::shared_ptr<ColorWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::ViewportModel>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            DJV_API std::shared_ptr<ftk::CheckBox> getEnabledCheckBox() const;

        private:
            FTK_PRIVATE();
        };

        class DJV_API_TYPE LevelsWidget : public ftk::IContainer
        {
            FTK_NON_COPYABLE(LevelsWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<ftk::Settings>&,
                const std::shared_ptr<models::ViewportModel>&,
                const std::shared_ptr<IWidget>& parent);

            LevelsWidget();

        public:
            DJV_API virtual ~LevelsWidget();

            DJV_API static std::shared_ptr<LevelsWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<ftk::Settings>&,
                const std::shared_ptr<models::ViewportModel>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            DJV_API std::shared_ptr<ftk::CheckBox> getEnabledCheckBox() const;

        private:
            FTK_PRIVATE();
        };

        class DJV_API_TYPE ExposureWidget : public ftk::IContainer
        {
            FTK_NON_COPYABLE(ExposureWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::ViewportModel>&,
                const std::shared_ptr<IWidget>& parent);

            ExposureWidget();

        public:
            DJV_API virtual ~ExposureWidget();

            DJV_API static std::shared_ptr<ExposureWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::ViewportModel>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            DJV_API std::shared_ptr<ftk::CheckBox> getEnabledCheckBox() const;

        private:
            FTK_PRIVATE();
        };

        class DJV_API_TYPE SoftClipWidget : public ftk::IContainer
        {
            FTK_NON_COPYABLE(SoftClipWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::ViewportModel>&,
                const std::shared_ptr<IWidget>& parent);

            SoftClipWidget();

        public:
            DJV_API virtual ~SoftClipWidget();

            DJV_API static std::shared_ptr<SoftClipWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::ViewportModel>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            DJV_API std::shared_ptr<ftk::CheckBox> getEnabledCheckBox() const;

        private:
            FTK_PRIVATE();
        };
    }
}
