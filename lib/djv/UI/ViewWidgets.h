// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/App/ViewTool.h>
#include <djv/Models/ViewportModel.h>

#include <tlRender/UI/Viewport.h>
#include <tlRender/Timeline/BackgroundOptions.h>
#include <tlRender/Timeline/DisplayOptions.h>
#include <tlRender/Timeline/ForegroundOptions.h>

namespace djv
{
    namespace ui
    {
        //! View position and zoom widget.
        class ViewPosZoomWidget : public ftk::IWidget
        {
            FTK_NON_COPYABLE(ViewPosZoomWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<tl::ui::Viewport>&,
                const std::shared_ptr<IWidget>& parent);

            ViewPosZoomWidget();

        public:
            virtual ~ViewPosZoomWidget();

            static std::shared_ptr<ViewPosZoomWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<tl::ui::Viewport>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            ftk::Size2I getSizeHint() const override;
            void setGeometry(const ftk::Box2I&) override;

        private:
            FTK_PRIVATE();
        };

        //! View options widget.
        class ViewOptionsWidget : public ftk::IWidget
        {
            FTK_NON_COPYABLE(ViewOptionsWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::ViewportModel>&,
                const std::shared_ptr<IWidget>& parent);

            ViewOptionsWidget();

        public:
            virtual ~ViewOptionsWidget();

            static std::shared_ptr<ViewOptionsWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::ViewportModel>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            ftk::Size2I getSizeHint() const override;
            void setGeometry(const ftk::Box2I&) override;

        private:
            FTK_PRIVATE();
        };

        //! Aspect ratio widget.
        class AspectRatioWidget : public ftk::IWidget
        {
            FTK_NON_COPYABLE(AspectRatioWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<IWidget>& parent);

            AspectRatioWidget();

        public:
            virtual ~AspectRatioWidget();

            static std::shared_ptr<AspectRatioWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            const tl::AspectRatioOptions& getValue() const;
            void setValue(const tl::AspectRatioOptions&);
            void setCallback(const std::function<void(const tl::AspectRatioOptions&)>&);

            ftk::Size2I getSizeHint() const override;
            void setGeometry(const ftk::Box2I&) override;

        private:
            void _widgetUpdate();

            FTK_PRIVATE();
        };

        //! View aspect ratio widget.
        class ViewAspectRatioWidget : public ftk::IWidget
        {
            FTK_NON_COPYABLE(ViewAspectRatioWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::ViewportModel>&,
                const std::shared_ptr<IWidget>& parent);

            ViewAspectRatioWidget();

        public:
            virtual ~ViewAspectRatioWidget();

            static std::shared_ptr<ViewAspectRatioWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::ViewportModel>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            ftk::Size2I getSizeHint() const override;
            void setGeometry(const ftk::Box2I&) override;

        private:
            void _widgetUpdate(const models::AspectRatioOptions&);

            FTK_PRIVATE();
        };

        //! View background widget.
        class ViewBackgroundWidget : public ftk::IWidget
        {
            FTK_NON_COPYABLE(ViewBackgroundWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::ViewportModel>&,
                const std::shared_ptr<IWidget>& parent);

            ViewBackgroundWidget();

        public:
            virtual ~ViewBackgroundWidget();

            static std::shared_ptr<ViewBackgroundWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::ViewportModel>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            ftk::Size2I getSizeHint() const override;
            void setGeometry(const ftk::Box2I&) override;

        private:
            void _optionsUpdate(const tl::BackgroundOptions&);

            FTK_PRIVATE();
        };

        //! View outline widget.
        class ViewOutlineWidget : public ftk::IWidget
        {
            FTK_NON_COPYABLE(ViewOutlineWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::ViewportModel>&,
                const std::shared_ptr<IWidget>& parent);

            ViewOutlineWidget();

        public:
            virtual ~ViewOutlineWidget();

            static std::shared_ptr<ViewOutlineWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::ViewportModel>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            ftk::Size2I getSizeHint() const override;
            void setGeometry(const ftk::Box2I&) override;

        private:
            FTK_PRIVATE();
        };

        //! View grid widget.
        class ViewGridWidget : public ftk::IWidget
        {
            FTK_NON_COPYABLE(ViewGridWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::ViewportModel>&,
                const std::shared_ptr<IWidget>& parent);

            ViewGridWidget();

        public:
            virtual ~ViewGridWidget();

            static std::shared_ptr<ViewGridWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::ViewportModel>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            ftk::Size2I getSizeHint() const override;
            void setGeometry(const ftk::Box2I&) override;

        private:
            FTK_PRIVATE();
        };

        //! View center marker widget.
        class ViewCenterMarkerWidget : public ftk::IWidget
        {
            FTK_NON_COPYABLE(ViewCenterMarkerWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::ViewportModel>&,
                const std::shared_ptr<IWidget>& parent);

            ViewCenterMarkerWidget();

        public:
            virtual ~ViewCenterMarkerWidget();

            static std::shared_ptr<ViewCenterMarkerWidget> create(
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
