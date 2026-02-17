// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/ColorTool.h>

#include <djv/App/App.h>
#include <djv/UI/ColorWidgets.h>

#include <ftk/UI/RowLayout.h>
#include <ftk/UI/ScrollWidget.h>

namespace djv
{
    namespace app
    {
        struct ColorTool::Private
        {
            std::map<std::string, std::shared_ptr<ftk::Bellows> > bellows;
        };

        void ColorTool::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            IToolWidget::_init(
                context,
                app,
                models::Tool::Color,
                "djv::app::ColorTool",
                parent);
            FTK_P();

#if defined(TLRENDER_OCIO)
            auto colorModel = app->getColorModel();
            auto ocioWidget = ui::OCIOWidget::create(context, colorModel);
            auto lutWidget = ui::LUTWidget::create(context, colorModel);
#endif // TLRENDER_OCIO
            auto viewportModel = app->getViewportModel();
            auto colorWidget = ui::ColorWidget::create(context, viewportModel);
            auto levelsWidget = ui::LevelsWidget::create(context, app->getSettings(), viewportModel);
            auto exrDisplayWidget = ui::EXRDisplayWidget::create(context, viewportModel);
            auto softClipWidget = ui::SoftClipWidget::create(context, viewportModel);

            auto layout = ftk::VerticalLayout::create(context);
            layout->setSpacingRole(ftk::SizeRole::None);
#if defined(TLRENDER_OCIO)
            p.bellows["OCIO"] = ftk::Bellows::create(context, "OCIO", layout);
            p.bellows["OCIO"]->setWidget(ocioWidget);
            p.bellows["LUT"] = ftk::Bellows::create(context, "LUT", layout);
            p.bellows["LUT"]->setWidget(lutWidget);
#endif // TLRENDER_OCIO
            p.bellows["Color"] = ftk::Bellows::create(context, "Color", layout);
            p.bellows["Color"]->setWidget(colorWidget);
            p.bellows["Levels"] = ftk::Bellows::create(context, "Levels", layout);
            p.bellows["Levels"]->setWidget(levelsWidget);
            p.bellows["EXRDisplay"] = ftk::Bellows::create(context, "EXR Display", layout);
            p.bellows["EXRDisplay"]->setWidget(exrDisplayWidget);
            p.bellows["SoftClip"] = ftk::Bellows::create(context, "Soft Clip", layout);
            p.bellows["SoftClip"]->setWidget(softClipWidget);
            auto scrollWidget = ftk::ScrollWidget::create(context);
            scrollWidget->setBorder(false);
            scrollWidget->setWidget(layout);
            _setWidget(scrollWidget);

            _loadSettings(p.bellows);
        }

        ColorTool::ColorTool() :
            _p(new Private)
        {}

        ColorTool::~ColorTool()
        {
            _saveSettings(_p->bellows);
        }

        std::shared_ptr<ColorTool> ColorTool::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<ColorTool>(new ColorTool);
            out->_init(context, app, parent);
            return out;
        }
    }
}
