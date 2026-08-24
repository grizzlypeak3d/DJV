// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/ExportTool.h>

#include <djv/App/App.h>
#include <djv/UI/ExportWidget.h>

#include <tlRender/Timeline/Player.h>

namespace djv
{
    namespace app
    {
        struct ExportTool::Private
        {
            std::shared_ptr<ui::ExportWidget> widget;

            std::shared_ptr<ftk::Observer<std::shared_ptr<tl::Player> > > playerObserver;
        };

        void ExportTool::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<MainWindow>& mainWindow,
            const std::shared_ptr<IWidget>& parent)
        {
            IToolWidget::_init(
                context,
                app,
                mainWindow,
                "Export",
                "Export",
                "djv::app::ExportTool",
                parent);
            FTK_P();

            p.widget = ui::ExportWidget::create(
                context,
                app->getFilesModel(),
                app->getColorModel(),
                app->getViewportModel(),
                app->getSettingsModel(),
                app->getTimeUnitsModel());

            _setWidget(p.widget);

            p.playerObserver = ftk::Observer<std::shared_ptr<tl::Player> >::create(
                app->observePlayer(),
                [this](const std::shared_ptr<tl::Player>& value)
                {
                    _p->widget->setPlayer(value);
                });
        }

        ExportTool::ExportTool() :
            _p(new Private)
        {}

        ExportTool::~ExportTool()
        {}

        std::shared_ptr<ExportTool> ExportTool::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<MainWindow>& mainWindow,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<ExportTool>(new ExportTool);
            out->_init(context, app, mainWindow, parent);
            return out;
        }
    }
}
