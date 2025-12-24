// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djvApp/Models/SettingsModel.h>

#include <tlRender/Timeline/Player.h>

#include <ftk/UI/Window.h>

namespace ftk
{
    class MenuBar;
}

namespace tl
{
    namespace ui
    {
        class TimelineWidget;
    }
}

namespace djv
{
    namespace app
    {
        class App;
        class Viewport;

        //! Main window.
        class MainWindow : public ftk::Window
        {
            FTK_NON_COPYABLE(MainWindow);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&);

            MainWindow();

        public:
            ~MainWindow();

            //! Create a new main window.
            static std::shared_ptr<MainWindow> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&);

            //! Get the menu bar.
            const std::shared_ptr<ftk::MenuBar> getMenuBar() const;

            //! Get the viewport.
            const std::shared_ptr<Viewport>& getViewport() const;

            //! Get the timeline widget.
            const std::shared_ptr<tl::ui::TimelineWidget>& getTimelineWidget() const;

            //! Focus the current frame widget.
            void focusCurrentFrame();

            //! Show the about dialog.
            void showAboutDialog();

            void setGeometry(const ftk::Box2I&) override;
            void keyPressEvent(ftk::KeyEvent&) override;
            void keyReleaseEvent(ftk::KeyEvent&) override;
            void dropEvent(ftk::DragDropEvent&) override;

        private:
            void _settingsUpdate(const MouseSettings&);
            void _settingsUpdate(const TimelineSettings&);
            void _settingsUpdate(const WindowSettings&);

            FTK_PRIVATE();
        };
    }
}
