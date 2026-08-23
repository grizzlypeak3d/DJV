// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/App/Export.h>
#include <djv/Models/Export.h>

#include <tlRender/Timeline/Player.h>

#include <ftk/UI/Window.h>

namespace djv
{
    namespace app
    {
        class App;
        class Viewport;

        //! Secondary window.
        class DJV_APP_API_TYPE SecondaryWindow : public ftk::Window
        {
            FTK_NON_COPYABLE(SecondaryWindow);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<ftk::Window>& shared);

            SecondaryWindow();

        public:
            DJV_APP_API virtual ~SecondaryWindow();

            DJV_APP_API static std::shared_ptr<SecondaryWindow> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<ftk::Window>& shared = nullptr);

            //! Get the viewport.
            DJV_APP_API const std::shared_ptr<Viewport>& getViewport() const;

            //! Set the view.
            DJV_APP_API void setView(
                const ftk::V2I& pos,
                double          zoom,
                bool            frame);

            DJV_APP_API void keyPressEvent(ftk::KeyEvent&) override;
            DJV_APP_API void keyReleaseEvent(ftk::KeyEvent&) override;

        private:
            FTK_PRIVATE();
        };
    }
}
