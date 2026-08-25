// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/InfoTool.h>

#include <djv/App/App.h>
#include <djv/UI/InfoWidget.h>

#include <tlRender/Timeline/Player.h>

namespace djv
{
    namespace app
    {
        struct InfoTool::Private
        {
            std::shared_ptr<ui::InfoWidget> widget;

            std::shared_ptr<ftk::Observer<std::shared_ptr<tl::Player> > > playerObserver;
        };

        void InfoTool::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<MainWindow>& mainWindow,
            const std::shared_ptr<IWidget>& parent)
        {
            IToolWidget::_init(
                context,
                app,
                mainWindow,
                "Information",
                "Info",
                "djv::app::InfoTool",
                parent);
            FTK_P();

            p.widget = ui::InfoWidget::create(context, app->getSettings());
            _setWidget(p.widget);

            p.playerObserver = ftk::Observer<std::shared_ptr<tl::Player> >::create(
                app->observePlayer(),
                [this](const std::shared_ptr<tl::Player>& value)
                {
                    _p->widget->setPlayer(value);
                });
        }

        InfoTool::InfoTool() :
            _p(new Private)
        {}

        InfoTool::~InfoTool()
        {}

        std::shared_ptr<InfoTool> InfoTool::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<MainWindow>& mainWindow,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<InfoTool>(new InfoTool);
            out->_init(context, app, mainWindow, parent);
            return out;
        }
    }
}
