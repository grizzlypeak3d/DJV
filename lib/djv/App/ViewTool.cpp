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
            std::shared_ptr<ui::ViewPosZoomWidget> posZoomWidget;
            std::shared_ptr<ui::ViewOptionsWidget> optionsWidget;
            std::shared_ptr<ui::ViewAspectRatioWidget> aspectRatioWidget;
            std::shared_ptr<ui::ViewBackgroundWidget> backgroundWidget;
            std::shared_ptr<ui::ViewOutlineWidget> outlineWidget;
            std::shared_ptr<ui::ViewGridWidget> gridWidget;
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

            p.posZoomWidget = ui::ViewPosZoomWidget::create(context, mainWindow->getViewport());
            auto viewportModel = app->getViewportModel();
            p.optionsWidget = ui::ViewOptionsWidget::create(context, viewportModel);
            p.aspectRatioWidget = ui::ViewAspectRatioWidget::create(context, viewportModel);
            p.backgroundWidget = ui::ViewBackgroundWidget::create(context, viewportModel);
            p.outlineWidget = ui::ViewOutlineWidget::create(context, viewportModel);
            p.gridWidget = ui::ViewGridWidget::create(context, viewportModel);

            auto layout = ftk::VerticalLayout::create(context);
            layout->setSpacingRole(ftk::SizeRole::None);
            p.bellows["PosZoom"] = ftk::Bellows::create(context, "Position and Zoom", layout);
            p.bellows["PosZoom"]->setWidget(p.posZoomWidget);
            p.bellows["Options"] = ftk::Bellows::create(context, "Options", layout);
            p.bellows["Options"]->setWidget(p.optionsWidget);
            p.bellows["AspectRatio"] = ftk::Bellows::create(context, "Aspect Ratio", layout);
            p.bellows["AspectRatio"]->setWidget(p.aspectRatioWidget);
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
