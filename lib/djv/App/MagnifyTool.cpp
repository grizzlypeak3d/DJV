// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/MagnifyTool.h>

#include <djv/App/App.h>
#include <djv/App/MainWindow.h>
#include <djv/UI/MagnifyWidget.h>
#include <djv/UI/Viewport.h>

#include <tlRender/Timeline/Player.h>

namespace djv
{
    namespace app
    {
        struct MagnifyTool::Private
        {
            std::shared_ptr<ui::MagnifyWidget> widget;

            std::shared_ptr<ftk::Observer<std::shared_ptr<tl::Player> > > playerObserver;
        };

        void MagnifyTool::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<MainWindow>& mainWindow,
            const std::shared_ptr<IWidget>& parent)
        {
            IToolWidget::_init(
                context,
                app,
                mainWindow,
                "Magnify",
                "Magnify",
                "djv::app::MagnifyTool",
                parent);
            FTK_P();

            p.widget = ui::MagnifyWidget::create(
                context,
                app->getSettings(),
                mainWindow->getViewport(),
                app->getFilesModel(),
                app->getColorModel(),
                app->getViewportModel(),
                app->getSettingsModel());

            // The magnified view has no natural size of its own, so this
            // takes what room is left rather than a band of its own.
            setVStretch(ftk::Stretch::Expanding);
            _setWidget(p.widget);

            p.playerObserver = ftk::Observer<std::shared_ptr<tl::Player> >::create(
                app->observePlayer(),
                [this](const std::shared_ptr<tl::Player>& value)
                {
                    _p->widget->setPlayer(value);
                });
        }

        MagnifyTool::MagnifyTool() :
            _p(new Private)
        {}

        MagnifyTool::~MagnifyTool()
        {}

        std::shared_ptr<MagnifyTool> MagnifyTool::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<MainWindow>& mainWindow,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<MagnifyTool>(new MagnifyTool);
            out->_init(context, app, mainWindow, parent);
            return out;
        }
    }
}
