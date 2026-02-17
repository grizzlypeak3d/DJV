// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/SecondaryWindow.h>

#include <djv/App/App.h>
#include <djv/App/MainWindow.h>
#include <djv/App/Viewport.h>

#include <ftk/UI/MenuBar.h>
#include <ftk/Core/Format.h>

namespace djv
{
    namespace app
    {
        struct SecondaryWindow::Private
        {
            std::weak_ptr<App> app;

            std::shared_ptr<Viewport> viewport;

            std::shared_ptr<ftk::Observer<std::shared_ptr<tl::Player> > > playerObserver;
        };

        void SecondaryWindow::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<ftk::Window>& shared)
        {
            Window::_init(
                context,
                app,
                ftk::Format("{0} {1} - {2}").arg("DJV").arg(DJV_VERSION_FULL).arg("Secondary"),
                ftk::Size2I(1280, 960));
            FTK_P();

            p.app = app;

            p.viewport = Viewport::create(context, app);
            p.viewport->setParent(shared_from_this());

            p.playerObserver = ftk::Observer<std::shared_ptr<tl::Player> >::create(
                app->observePlayer(),
                [this](const std::shared_ptr<tl::Player>& value)
                {
                    _p->viewport->setPlayer(value);
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
            const std::shared_ptr<ftk::Window>& shared)
        {
            auto out = std::shared_ptr<SecondaryWindow>(new SecondaryWindow);
            out->_init(context, app, shared);
            return out;
        }

        const std::shared_ptr<Viewport>& SecondaryWindow::getViewport() const
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
                event.accept = true;
                close();
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
