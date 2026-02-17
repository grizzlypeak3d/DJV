// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/DiagTool.h>

#include <djv/App/App.h>

#include <tlRender/Timeline/Player.h>
#include <tlRender/IO/Plugin.h>

#include <ftk/UI/DiagWidget.h>
#include <ftk/UI/GraphWidget.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/ScrollWidget.h>
#include <ftk/Core/Format.h>
#include <ftk/Core/String.h>
#include <ftk/Core/Timer.h>

namespace djv
{
    namespace app
    {
        struct DiagTool::Private
        {
            std::shared_ptr<ftk::DiagWidget> diagWidget;
        };

        void DiagTool::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            IToolWidget::_init(
                context,
                app,
                models::Tool::Diag,
                "djv::app::DiagTool",
                parent);
            FTK_P();

            p.diagWidget = ftk::DiagWidget::create(context, app->getDiagModel());
            p.diagWidget->setMarginRole(ftk::SizeRole::Margin);

            auto scrollWidget = ftk::ScrollWidget::create(context);
            scrollWidget->setWidget(p.diagWidget);
            scrollWidget->setBorder(false);
            scrollWidget->setVStretch(ftk::Stretch::Expanding);
            _setWidget(scrollWidget);
        }

        DiagTool::DiagTool() :
            _p(new Private)
        {}

        DiagTool::~DiagTool()
        {}

        std::shared_ptr<DiagTool> DiagTool::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<DiagTool>(new DiagTool);
            out->_init(context, app, parent);
            return out;
        }
    }
}
