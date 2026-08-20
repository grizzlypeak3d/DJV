// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/Models/Export.h>
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
        class DJV_API_TYPE MainWindow : public ftk::Window
        {
            FTK_NON_COPYABLE(MainWindow);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&);

            MainWindow();

        public:
            DJV_API ~MainWindow();

            //! Create a new main window.
            DJV_API static std::shared_ptr<MainWindow> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&);

            //! Get the menu bar.
            DJV_API const std::shared_ptr<ftk::MenuBar> getMenuBar() const;

            //! Get the viewport.
            DJV_API const std::shared_ptr<Viewport>& getViewport() const;

            //! Send a mouse click at the given window position. For the
            //! capture harness: a click routed the way a real one is, through
            //! hit testing and the widget's own handlers, rather than by
            //! calling the handler that is assumed to be the right one.
            DJV_API void click(const ftk::V2I&, int modifiers = 0);

            //! Get the timeline widget.
            DJV_API const std::shared_ptr<tl::ui::TimelineWidget>& getTimelineWidget() const;

            //! Get the active tool widget, or null if no tool is active.
            //! Get an open tool by name, or null when it is not open.
            DJV_API std::shared_ptr<IToolWidget> getToolWidget(const std::string&) const;

            //! Set the two splitter positions (0-1). The settings store these
            //! but only apply them at construction, so this moves the widgets
            //! directly -- used by the documentation screenshot capture.
            DJV_API void setSplitters(float splitter, float splitter2);

            //! Get whether presentation mode is enabled.
            DJV_API bool hasPresentMode() const;

            //! Observe whether presentation mode is enabled.
            DJV_API std::shared_ptr<ftk::IObservable<bool> > observePresentMode() const;

            //! Set whether presentation mode is enabled.
            DJV_API void setPresentMode(bool);

            //! Focus the current frame widget.
            DJV_API void focusCurrentFrame();

            //! Show the about dialog.
            DJV_API void showAboutDialog();

            //! Show the system information dialog.
            DJV_API void showSysInfoDialog();

            DJV_API void setGeometry(const ftk::Box2I&) override;
            DJV_API void keyPressEvent(ftk::KeyEvent&) override;
            DJV_API void keyReleaseEvent(ftk::KeyEvent&) override;
            DJV_API void dropEvent(ftk::DragDropEvent&) override;

        private:
            void _settingsUpdate(const models::MouseSettings&);
            void _settingsUpdate(const models::TimelineSettings&);
            void _windowUpdate();

            FTK_PRIVATE();
        };
    }
}
