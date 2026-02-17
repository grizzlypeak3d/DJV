// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/ViewToolBar.h>

#include <djv/App/MainWindow.h>
#include <djv/App/ViewActions.h>
#include <djv/App/Viewport.h>

#include <ftk/UI/DoubleEdit.h>
#include <ftk/UI/RowLayout.h>

namespace djv
{
    namespace app
    {
        struct ViewToolBar::Private
        {
            std::shared_ptr<ftk::DoubleEdit> zoomEdit;
            std::shared_ptr<ftk::DoubleResetButton> zoomReset;

            std::shared_ptr<ftk::Observer<std::pair<ftk::V2I, double> > > posZoomObserver;
            bool updating = false;
        };

        void ViewToolBar::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<MainWindow>& mainWindow,
            const std::shared_ptr<ViewActions>& viewActions,
            const std::shared_ptr<IWidget>& parent)
        {
            ToolBar::_init(context, ftk::Orientation::Horizontal, parent);
            FTK_P();

            setMarginRole(ftk::SizeRole::MarginInside);

            auto actions = viewActions->getActions();
            addAction(actions["Frame"]);

            p.zoomEdit = ftk::DoubleEdit::create(context);
            p.zoomEdit->setRange(0.0001, 1000.0);
            p.zoomEdit->setStep(0.1);
            p.zoomEdit->setLargeStep(0.5);
            p.zoomEdit->setDefaultValue(1.0);
            p.zoomEdit->setPrecision(2);
            p.zoomEdit->setBorderRole(ftk::ColorRole::None);
            p.zoomEdit->setTooltip("View zoom");
            addWidget(p.zoomEdit);

            addAction(actions["ZoomReset"]);

            std::weak_ptr<MainWindow> mainWindowWeak(mainWindow);
            p.zoomEdit->setCallback(
                [this, mainWindowWeak](double value)
                {
                    if (!_p->updating)
                    {
                        if (auto mainWindow = mainWindowWeak.lock())
                        {
                            auto viewport = mainWindow->getViewport();
                            const ftk::Box2I& g = viewport->getGeometry();
                            const ftk::V2I focus(g.w() / 2, g.h() / 2);
                            mainWindow->getViewport()->setViewZoom(value, focus);
                        }
                    }
                });

            p.posZoomObserver = ftk::Observer<std::pair<ftk::V2I, double> >::create(
                mainWindow->getViewport()->observeViewPosAndZoom(),
                [this](const std::pair<ftk::V2I, double>& value)
                {
                    FTK_P();
                    p.updating = true;
                    p.zoomEdit->setValue(value.second);
                    p.updating = false;
                });
        }

        ViewToolBar::ViewToolBar() :
            _p(new Private)
        {}

        ViewToolBar::~ViewToolBar()
        {}

        std::shared_ptr<ViewToolBar> ViewToolBar::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<MainWindow>& mainWindow,
            const std::shared_ptr<ViewActions>& viewActions,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<ViewToolBar>(new ViewToolBar);
            out->_init(context, mainWindow, viewActions, parent);
            return out;
        }
    }
}
