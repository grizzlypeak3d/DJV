// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/Models/OCIOModel.h>

#include <ftk/UI/IWidget.h>

namespace ftk
{
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
        class OCIOWidget : public ftk::IWidget
        {
            FTK_NON_COPYABLE(OCIOWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::ColorModel>&,
                const std::shared_ptr<IWidget>& parent);

            OCIOWidget();

        public:
            virtual ~OCIOWidget();

            static std::shared_ptr<OCIOWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::ColorModel>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            ftk::Size2I getSizeHint() const override;
            void setGeometry(const ftk::Box2I&) override;

        private:
            FTK_PRIVATE();
        };

        class LUTWidget : public ftk::IWidget
        {
            FTK_NON_COPYABLE(LUTWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::ColorModel>&,
                const std::shared_ptr<IWidget>& parent);

            LUTWidget();

        public:
            virtual ~LUTWidget();

            static std::shared_ptr<LUTWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::ColorModel>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            ftk::Size2I getSizeHint() const override;
            void setGeometry(const ftk::Box2I&) override;

        private:
            FTK_PRIVATE();
        };

        class ColorWidget : public ftk::IWidget
        {
            FTK_NON_COPYABLE(ColorWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::ViewportModel>&,
                const std::shared_ptr<IWidget>& parent);

            ColorWidget();

        public:
            virtual ~ColorWidget();

            static std::shared_ptr<ColorWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::ViewportModel>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            ftk::Size2I getSizeHint() const override;
            void setGeometry(const ftk::Box2I&) override;

        private:
            FTK_PRIVATE();
        };

        class LevelsWidget : public ftk::IWidget
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
            virtual ~LevelsWidget();

            static std::shared_ptr<LevelsWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<ftk::Settings>&,
                const std::shared_ptr<models::ViewportModel>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            ftk::Size2I getSizeHint() const override;
            void setGeometry(const ftk::Box2I&) override;

        private:
            FTK_PRIVATE();
        };

        class EXRDisplayWidget : public ftk::IWidget
        {
            FTK_NON_COPYABLE(EXRDisplayWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::ViewportModel>&,
                const std::shared_ptr<IWidget>& parent);

            EXRDisplayWidget();

        public:
            virtual ~EXRDisplayWidget();

            static std::shared_ptr<EXRDisplayWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::ViewportModel>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            ftk::Size2I getSizeHint() const override;
            void setGeometry(const ftk::Box2I&) override;

        private:
            FTK_PRIVATE();
        };

        class SoftClipWidget : public ftk::IWidget
        {
            FTK_NON_COPYABLE(SoftClipWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::ViewportModel>&,
                const std::shared_ptr<IWidget>& parent);

            SoftClipWidget();

        public:
            virtual ~SoftClipWidget();

            static std::shared_ptr<SoftClipWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::ViewportModel>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            ftk::Size2I getSizeHint() const override;
            void setGeometry(const ftk::Box2I&) override;

        private:
            FTK_PRIVATE();
        };
    }
}
