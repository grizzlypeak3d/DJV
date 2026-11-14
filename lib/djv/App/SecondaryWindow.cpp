// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/SecondaryWindow.h>

#include <djv/Models/DisplaysModel.h>

#include <djv/App/App.h>
#include <djv/Models/ViewportModel.h>
#include <djv/App/MainWindow.h>
#include <djv/UI/Viewport.h>
#include <djv/Models/AppInfoModel.h>

#include <ftk/UI/MenuBar.h>
#include <ftk/Core/Format.h>

namespace djv
{
    namespace app
    {
        struct SecondaryWindow::Private
        {
            std::weak_ptr<App> app;

            std::shared_ptr<ui::Viewport> viewport;

            std::shared_ptr<ftk::Observer<std::shared_ptr<tl::Player> > > playerObserver;
            std::shared_ptr<ftk::Observer<bool> > ditherObserver;
            std::shared_ptr<ftk::Observer<ftk::ImageType> > colorBufferObserver;
            std::shared_ptr<ftk::Observer<models::DisplayColor> > displayColorObserver;
        };

        void SecondaryWindow::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<ftk::Window>& shared,
            int screen,
            const ftk::Size2I& size)
        {
            Window::_init(
                context,
                app,
                ftk::Format("{0} {1} {2}").
                    arg(app->getAppInfoModel()->getFullName()).
                    arg(app->getAppInfoModel()->getVersion()).
                    arg("Secondary"),
                size,
                screen);
            FTK_P();

            p.app = app;

            p.viewport = ui::Viewport::create(
                context,
                app->getFilesModel(),
                app->getColorModel(),
                app->getViewportModel(),
                app->getTimeUnitsModel(),
                app->getSettingsModel(),
                app->getAnnotationsModel(),
                app->getDrawModel(),
                app->getSysLogModel());
            p.viewport->setParent(shared_from_this());

            p.playerObserver = ftk::Observer<std::shared_ptr<tl::Player> >::create(
                app->observePlayer(),
                [this](const std::shared_ptr<tl::Player>& value)
                {
                    _p->viewport->setPlayer(value);
                });

            p.ditherObserver = ftk::Observer<bool>::create(
                app->getViewportModel()->observeDither(),
                [this](bool value)
                {
                    setDither(value);
                });

            // The same buffer as the main window's: without it the window
            // kept the default, eight bits on OpenGL ES and half float
            // elsewhere, whatever the settings chose.
            p.colorBufferObserver = ftk::Observer<ftk::ImageType>::create(
                app->getViewportModel()->observeColorBuffer(),
                [this](ftk::ImageType value)
                {
                    setBufferType(ftk::ImageType::RGBA_U8 == value ?
                        ftk::WindowBufferType::U8 :
                        ftk::WindowBufferType::F16);
                });

            // Its own display and view, when one is chosen for it in the
            // Displays tool.
            p.displayColorObserver = ftk::Observer<models::DisplayColor>::create(
                app->getDisplaysModel()->observeSecondaryColor(),
                [this](const models::DisplayColor& value)
                {
                    _p->viewport->setDisplayColor(value);
                });
        }

        SecondaryWindow::SecondaryWindow() :
            _p(new Private)
        {}

        SecondaryWindow::~SecondaryWindow()
        {
            _makeCurrent();
            _p->viewport->setParent(nullptr);
        }

        std::shared_ptr<SecondaryWindow> SecondaryWindow::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<ftk::Window>& shared,
            int screen,
            const ftk::Size2I& size)
        {
            auto out = std::shared_ptr<SecondaryWindow>(new SecondaryWindow);
            out->_init(context, app, shared, screen, size);
            return out;
        }

        const std::shared_ptr<ui::Viewport>& SecondaryWindow::getViewport() const
        {
            return _p->viewport;
        }

        void SecondaryWindow::setView(
            const ftk::V2I& pos,
            double zoom,
            bool frame)
        {
            FTK_P();
            p.viewport->setViewPosAndZoom(pos, zoom);
            p.viewport->setFrameView(frame);
        }

        void SecondaryWindow::keyPressEvent(ftk::KeyEvent& event)
        {
            FTK_P();
            if (auto app = p.app.lock())
            {
                auto menuBar = app->getMainWindow()->getMenuBar();
                event.accept = menuBar->shortcut(event.key, event.modifiers);
            }
            if (!event.accept &&
                ftk::Key::Escape == event.key &&
                0 == event.modifiers)
            {
                // Filling a monitor, Escape gives a window back rather than
                // closing it: the way out of full screen everywhere else.
                event.accept = true;
                auto app = p.app.lock();
                if (app && !app->getDisplaysModel()->getSecondary().monitor.empty())
                {
                    app->getDisplaysModel()->setSecondaryMonitor(std::string());
                }
                else
                {
                    close();
                }
            }
            if (!event.accept)
            {
                Window::keyPressEvent(event);
            }
        }

        void SecondaryWindow::keyReleaseEvent(ftk::KeyEvent& event)
        {
            event.accept = true;
        }
    }
}
