// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/UI/Export.h>
#include <djv/App/ViewTool.h>
#include <djv/Models/Export.h>
#include <djv/Models/ViewportModel.h>

#include <tlRender/UI/Viewport.h>
#include <tlRender/Timeline/BackgroundOptions.h>
#include <tlRender/Timeline/DisplayOptions.h>
#include <tlRender/Timeline/ForegroundOptions.h>

namespace ftk
{
    class CheckBox;
}

namespace djv
{
    namespace ui
    {
        //! View options widget.
        class DJV_UI_API_TYPE ViewOptionsWidget : public ftk::IContainer
        {
            FTK_NON_COPYABLE(ViewOptionsWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::ViewportModel>&,
                const std::shared_ptr<IWidget>& parent);

            ViewOptionsWidget();

        public:
            DJV_UI_API virtual ~ViewOptionsWidget();

            DJV_UI_API static std::shared_ptr<ViewOptionsWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::ViewportModel>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

        private:
            FTK_PRIVATE();
        };

        //! Aspect ratio widget.
        class DJV_UI_API_TYPE AspectRatioWidget : public ftk::IContainer
        {
            FTK_NON_COPYABLE(AspectRatioWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<IWidget>& parent);

            AspectRatioWidget();

        public:
            DJV_UI_API virtual ~AspectRatioWidget();

            DJV_UI_API static std::shared_ptr<AspectRatioWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            DJV_UI_API const tl::AspectRatioOptions& getValue() const;
            DJV_UI_API void setValue(const tl::AspectRatioOptions&);
            DJV_UI_API void setCallback(const std::function<void(const tl::AspectRatioOptions&)>&);

        private:
            void _widgetUpdate();

            FTK_PRIVATE();
        };

        //! View aspect ratio widget.
        class DJV_UI_API_TYPE ViewAspectRatioWidget : public ftk::IContainer
        {
            FTK_NON_COPYABLE(ViewAspectRatioWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::ViewportModel>&,
                const std::shared_ptr<IWidget>& parent);

            ViewAspectRatioWidget();

        public:
            DJV_UI_API virtual ~ViewAspectRatioWidget();

            DJV_UI_API static std::shared_ptr<ViewAspectRatioWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::ViewportModel>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

        private:
            void _widgetUpdate(const models::AspectRatioOptions&);

            FTK_PRIVATE();
        };

        //! View background widget.
        class DJV_UI_API_TYPE ViewBackgroundWidget : public ftk::IContainer
        {
            FTK_NON_COPYABLE(ViewBackgroundWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::ViewportModel>&,
                const std::shared_ptr<IWidget>& parent);

            ViewBackgroundWidget();

        public:
            DJV_UI_API virtual ~ViewBackgroundWidget();

            DJV_UI_API static std::shared_ptr<ViewBackgroundWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::ViewportModel>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

        private:
            void _optionsUpdate(const tl::BackgroundOptions&);

            FTK_PRIVATE();
        };

        //! View outline widget.
        class DJV_UI_API_TYPE ViewOutlineWidget : public ftk::IContainer
        {
            FTK_NON_COPYABLE(ViewOutlineWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::ViewportModel>&,
                const std::shared_ptr<IWidget>& parent);

            ViewOutlineWidget();

        public:
            DJV_UI_API virtual ~ViewOutlineWidget();

            DJV_UI_API static std::shared_ptr<ViewOutlineWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::ViewportModel>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            DJV_UI_API std::shared_ptr<ftk::CheckBox> getEnabledCheckBox() const;

        private:
            FTK_PRIVATE();
        };

        //! View grid widget.
        class DJV_UI_API_TYPE ViewGridWidget : public ftk::IContainer
        {
            FTK_NON_COPYABLE(ViewGridWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::ViewportModel>&,
                const std::shared_ptr<IWidget>& parent);

            ViewGridWidget();

        public:
            DJV_UI_API virtual ~ViewGridWidget();

            DJV_UI_API static std::shared_ptr<ViewGridWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::ViewportModel>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            DJV_UI_API std::shared_ptr<ftk::CheckBox> getEnabledCheckBox() const;

        private:
            FTK_PRIVATE();
        };

        //! View center marker widget.
        class DJV_UI_API_TYPE ViewCenterMarkerWidget : public ftk::IContainer
        {
            FTK_NON_COPYABLE(ViewCenterMarkerWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::ViewportModel>&,
                const std::shared_ptr<IWidget>& parent);

            ViewCenterMarkerWidget();

        public:
            DJV_UI_API virtual ~ViewCenterMarkerWidget();

            DJV_UI_API static std::shared_ptr<ViewCenterMarkerWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::ViewportModel>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            DJV_UI_API std::shared_ptr<ftk::CheckBox> getEnabledCheckBox() const;

        private:
            FTK_PRIVATE();
        };

        //! View HUD widget.
        class DJV_UI_API_TYPE ViewHUDWidget : public ftk::IContainer
        {
            FTK_NON_COPYABLE(ViewHUDWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::ViewportModel>&,
                const std::shared_ptr<IWidget>& parent);

            ViewHUDWidget();

        public:
            DJV_UI_API virtual ~ViewHUDWidget();

            DJV_UI_API static std::shared_ptr<ViewHUDWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::ViewportModel>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            DJV_UI_API std::shared_ptr<ftk::CheckBox> getEnabledCheckBox() const;

        private:
            FTK_PRIVATE();
        };
    }
}
