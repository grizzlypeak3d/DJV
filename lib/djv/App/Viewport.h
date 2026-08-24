// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/App/Export.h>
#include <djv/Models/Export.h>

#include <tlRender/UI/Viewport.h>

namespace djv
{
    namespace app
    {
        class App;

        //! Viewport.
        class DJV_APP_API_TYPE Viewport : public tl::ui::Viewport
        {
            FTK_NON_COPYABLE(Viewport);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<IWidget>& parent);

            Viewport();

        public:
            DJV_APP_API virtual ~Viewport();

            DJV_APP_API static std::shared_ptr<Viewport> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            DJV_APP_API void setPlayer(const std::shared_ptr<tl::Player>&) override;

            //! Set whether the toast stands in for the status bar.
            DJV_APP_API void setToastActive(bool);

            //! Set whether the heads up display is shown at all. Separate from
            //! the display's own options so that turning it off for
            //! presentation does not forget what was being shown.
            DJV_APP_API void setHUDActive(bool);

            DJV_APP_API ftk::Size2I getSizeHint() const override;
            DJV_APP_API void setGeometry(const ftk::Box2I&) override;
            DJV_APP_API void mouseMoveEvent(ftk::MouseMoveEvent&) override;
            DJV_APP_API void mousePressEvent(ftk::MouseClickEvent&) override;
            DJV_APP_API void mouseReleaseEvent(ftk::MouseClickEvent&) override;

        private:
            void _videoUpdate();
            void _toastUpdate();
            void _compareUpdate();
            void _hudUpdate();
            void _hudLayout();

            FTK_PRIVATE();
        };
    }
}

