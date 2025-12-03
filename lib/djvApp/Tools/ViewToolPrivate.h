// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djvApp/Tools/ViewTool.h>

#include <tlRender/Timeline/BackgroundOptions.h>
#include <tlRender/Timeline/DisplayOptions.h>
#include <tlRender/Timeline/ForegroundOptions.h>

namespace djv
{
    namespace app
    {
        class ViewPosZoomWidget : public ftk::IWidget
        {
            FTK_NON_COPYABLE(ViewPosZoomWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<MainWindow>&,
                const std::shared_ptr<IWidget>& parent);

            ViewPosZoomWidget();

        public:
            virtual ~ViewPosZoomWidget();

            static std::shared_ptr<ViewPosZoomWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<MainWindow>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            void setGeometry(const ftk::Box2I&) override;
            void sizeHintEvent(const ftk::SizeHintEvent&) override;

        private:
            FTK_PRIVATE();
        };

        class ViewOptionsWidget : public ftk::IWidget
        {
            FTK_NON_COPYABLE(ViewOptionsWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<IWidget>& parent);

            ViewOptionsWidget();

        public:
            virtual ~ViewOptionsWidget();

            static std::shared_ptr<ViewOptionsWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            void setGeometry(const ftk::Box2I&) override;
            void sizeHintEvent(const ftk::SizeHintEvent&) override;

        private:
            FTK_PRIVATE();
        };

        class BackgroundWidget : public ftk::IWidget
        {
            FTK_NON_COPYABLE(BackgroundWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<IWidget>& parent);

            BackgroundWidget();

        public:
            virtual ~BackgroundWidget();

            static std::shared_ptr<BackgroundWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            void setGeometry(const ftk::Box2I&) override;
            void sizeHintEvent(const ftk::SizeHintEvent&) override;

        private:
            void _optionsUpdate(const tl::timeline::BackgroundOptions&);

            FTK_PRIVATE();
        };

        class OutlineWidget : public ftk::IWidget
        {
            FTK_NON_COPYABLE(OutlineWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<IWidget>& parent);

            OutlineWidget();

        public:
            virtual ~OutlineWidget();

            static std::shared_ptr<OutlineWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            void setGeometry(const ftk::Box2I&) override;
            void sizeHintEvent(const ftk::SizeHintEvent&) override;

        private:
            void _optionsUpdate(const tl::timeline::ForegroundOptions&);

            FTK_PRIVATE();
        };

        class GridWidget : public ftk::IWidget
        {
            FTK_NON_COPYABLE(GridWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<IWidget>& parent);

            GridWidget();

        public:
            virtual ~GridWidget();

            static std::shared_ptr<GridWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            void setGeometry(const ftk::Box2I&) override;
            void sizeHintEvent(const ftk::SizeHintEvent&) override;

        private:
            FTK_PRIVATE();
        };
    }
}
