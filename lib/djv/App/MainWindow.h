// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/Models/SettingsModel.h>

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
        class IToolWidget;
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

            //! Get the active tool widget, or null if no tool is active.
            const std::shared_ptr<IToolWidget>& getToolWidget() const;

            //! Set the two splitter positions (0-1). The settings store these
            //! but only apply them at construction, so this moves the widgets
            //! directly -- used by the documentation screenshot capture.
            void setSplitters(float splitter, float splitter2);

            //! Get whether presentation mode is enabled.
            bool hasPresentMode() const;

            //! Observe whether presentation mode is enabled.
            std::shared_ptr<ftk::IObservable<bool> > observePresentMode() const;

            //! Set whether presentation mode is enabled.
            void setPresentMode(bool);

            //! Focus the current frame widget.
            void focusCurrentFrame();

            //! Show the about dialog.
            void showAboutDialog();

            //! Show the system information dialog.
            void showSysInfoDialog();

            void setGeometry(const ftk::Box2I&) override;
            void keyPressEvent(ftk::KeyEvent&) override;
            void keyReleaseEvent(ftk::KeyEvent&) override;
            void dropEvent(ftk::DragDropEvent&) override;

        private:
            void _settingsUpdate(const models::MouseSettings&);
            void _settingsUpdate(const models::TimelineSettings&);
            void _windowUpdate();

            FTK_PRIVATE();
        };
    }
}
