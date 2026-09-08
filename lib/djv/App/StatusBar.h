// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/App/Export.h>
#include <djv/Models/Export.h>

#include <tlRender/IO/IO.h>

#include <ftk/UI/IMouseWidget.h>
#include <ftk/Core/LogSystem.h>
#include <ftk/Core/Path.h>

namespace djv
{
    namespace app
    {
        class App;

        //! Status bar widget.
        class DJV_APP_API_TYPE StatusBar : public ftk::IMouseWidget
        {
            FTK_NON_COPYABLE(StatusBar);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<IWidget>& parent);

            StatusBar();

        public:
            DJV_APP_API virtual ~StatusBar();

            //! Create a new widget.
            DJV_APP_API static std::shared_ptr<StatusBar> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            //! Set a hint to show in place of the messages, e.g. the
            //! highlighted menu item's tooltip. An empty hint gives the
            //! messages back.
            DJV_APP_API void setHint(const std::string&);

            DJV_APP_API ftk::Size2I getSizeHint() const override;
            DJV_APP_API void setGeometry(const ftk::Box2I&) override;
            DJV_APP_API void mousePressEvent(ftk::MouseClickEvent&) override;
            DJV_APP_API void mouseReleaseEvent(ftk::MouseClickEvent&) override;

        private:
            void _infoUpdate(const ftk::Path&, const tl::IOInfo&);
            void _messagesUpdate();

            FTK_PRIVATE();
        };
    }
}
