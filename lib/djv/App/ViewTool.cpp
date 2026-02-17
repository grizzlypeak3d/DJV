// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/ViewTool.h>

#include <djv/UI/ViewWidgets.h>

#include <djv/App/App.h>
#include <djv/App/MainWindow.h>
#include <djv/App/Viewport.h>

#include <ftk/UI/RowLayout.h>
#include <ftk/UI/ScrollWidget.h>

namespace djv
{
    namespace app
    {
        struct ViewTool::Private
        {
            std::shared_ptr<ui::ViewPosZoomWidget> viewPosZoomWidget;
            std::shared_ptr<ui::ViewOptionsWidget> viewOptionsWidget;
            std::shared_ptr<ui::BackgroundWidget> backgroundWidget;
            std::shared_ptr<ui::OutlineWidget> outlineWidget;
            std::shared_ptr<ui::GridWidget> gridWidget;
            std::map<std::string, std::shared_ptr<ftk::Bellows> > bellows;
        };

        void ViewTool::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<MainWindow>& mainWindow,
            const std::shared_ptr<IWidget>& parent)
        {
            IToolWidget::_init(
                context,
                app,
                models::Tool::View,
                "djv::app::ViewTool",
                parent);
            FTK_P();

            p.viewPosZoomWidget = ui::ViewPosZoomWidget::create(context, mainWindow->getViewport());
            auto viewportModel = app->getViewportModel();
            p.viewOptionsWidget = ui::ViewOptionsWidget::create(context, viewportModel);
            p.backgroundWidget = ui::BackgroundWidget::create(context, viewportModel);
            p.outlineWidget = ui::OutlineWidget::create(context, viewportModel);
            p.gridWidget = ui::GridWidget::create(context, viewportModel);

            auto layout = ftk::VerticalLayout::create(context);
            layout->setSpacingRole(ftk::SizeRole::None);
            p.bellows["PosZoom"] = ftk::Bellows::create(context, "Position and Zoom", layout);
            p.bellows["PosZoom"]->setWidget(p.viewPosZoomWidget);
            p.bellows["Options"] = ftk::Bellows::create(context, "Options", layout);
            p.bellows["Options"]->setWidget(p.viewOptionsWidget);
            p.bellows["Background"] = ftk::Bellows::create(context, "Background", layout);
            p.bellows["Background"]->setWidget(p.backgroundWidget);
            p.bellows["Outline"] = ftk::Bellows::create(context, "Outline", layout);
            p.bellows["Outline"]->setWidget(p.outlineWidget);
            p.bellows["Grid"] = ftk::Bellows::create(context, "Grid", layout);
            p.bellows["Grid"]->setWidget(p.gridWidget);
            auto scrollWidget = ftk::ScrollWidget::create(context);
            scrollWidget->setBorder(false);
            scrollWidget->setWidget(layout);
            _setWidget(scrollWidget);

            _loadSettings(p.bellows);
        }

        ViewTool::ViewTool() :
            _p(new Private)
        {}

        ViewTool::~ViewTool()
        {
            _saveSettings(_p->bellows);
        }

        std::shared_ptr<ViewTool> ViewTool::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<MainWindow>& mainWindow,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<ViewTool>(new ViewTool);
            out->_init(context, app, mainWindow, parent);
            return out;
        }
    }
}
