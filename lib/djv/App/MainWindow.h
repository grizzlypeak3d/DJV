// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/App/Export.h>
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
    namespace ui
    {
        class Viewport;
    }

    namespace app
    {
        class App;
        class IToolWidget;

        //! Main window.
        class DJV_APP_API_TYPE MainWindow : public ftk::Window
        {
            FTK_NON_COPYABLE(MainWindow);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&);

            MainWindow();

        public:
            DJV_APP_API ~MainWindow();

            //! Create a new main window.
            DJV_APP_API static std::shared_ptr<MainWindow> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&);

            //! Get the menu bar.
            DJV_APP_API const std::shared_ptr<ftk::MenuBar> getMenuBar() const;

            //! Get the viewport.
            DJV_APP_API const std::shared_ptr<ui::Viewport>& getViewport() const;

            //! Send a mouse click at the given window position. For the
            //! capture harness: a click routed the way a real one is, through
            //! hit testing and the widget's own handlers, rather than by
            //! calling the handler that is assumed to be the right one.
            DJV_APP_API void click(const ftk::V2I&, int modifiers = 0);

            //! Get the timeline widget.
            DJV_APP_API const std::shared_ptr<tl::ui::TimelineWidget>& getTimelineWidget() const;

            //! Get the active tool widget, or null if no tool is active.
            //! Get an open tool by name, or null when it is not open.
            DJV_APP_API std::shared_ptr<IToolWidget> getToolWidget(const std::string&) const;

            //! Set the two splitter positions (0-1). The settings store these
            //! but only apply them at construction, so this moves the widgets
            //! directly -- used by the documentation screenshot capture.
            DJV_APP_API void setSplitters(float splitter, float splitter2);

            //! Get whether presentation mode is enabled.
            DJV_APP_API bool hasPresentMode() const;

            //! Observe whether presentation mode is enabled.
            DJV_APP_API std::shared_ptr<ftk::IObservable<bool> > observePresentMode() const;

            //! Set whether presentation mode is enabled.
            DJV_APP_API void setPresentMode(bool);

            //! Focus the current frame widget.
            DJV_APP_API void focusCurrentFrame();

            //! Open the review tool and start a new note, edited in place.
            DJV_APP_API void addReviewNote();

            //! Open the review tool and add a marker for the timeline
            //! in/out points.
            DJV_APP_API void addReviewRange();

            //! Save the window settings.
            DJV_APP_API void saveSettings();

            //! Show the about dialog.
            DJV_APP_API void showAboutDialog();

            //! Show the system information dialog.
            DJV_APP_API void showSysInfoDialog();

            DJV_APP_API void close() override;
            DJV_APP_API void setGeometry(const ftk::Box2I&) override;
            DJV_APP_API void keyPressEvent(ftk::KeyEvent&) override;
            DJV_APP_API void keyReleaseEvent(ftk::KeyEvent&) override;
            DJV_APP_API void dropEvent(ftk::DragDropEvent&) override;

        private:
            void _settingsUpdate(const models::MouseSettings&);
            void _settingsUpdate(const models::TimelineSettings&);
            void _windowUpdate();

            FTK_PRIVATE();
        };
    }
}
