// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/ColorTool.h>

#include <djv/App/App.h>
#include <djv/UI/ColorWidgets.h>

#include <ftk/UI/CheckBox.h>
#include <ftk/UI/Divider.h>
#include <ftk/UI/PushButton.h>
#include <ftk/UI/RowLayout.h>

namespace djv
{
    namespace app
    {
        struct ColorTool::Private
        {
            std::map<std::string, std::shared_ptr<ftk::Bellows> > bellows;
            std::shared_ptr<ftk::PushButton> resetButton;
        };

        void ColorTool::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<MainWindow>& mainWindow,
            const std::shared_ptr<IWidget>& parent)
        {
            IToolWidget::_init(
                context,
                app,
                mainWindow,
                "Color",
                "ColorControls",
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

            auto layout = ftk::VerticalLayout::create(context);
            layout->setSpacingRole(ftk::SizeRole::Border);
#if defined(TLRENDER_OCIO)
            p.bellows["OCIO"] = ftk::Bellows::create(context, "OCIO", layout);
            p.bellows["OCIO"]->setWidget(ocioWidget);
            p.bellows["OCIO"]->setToolWidget(ocioWidget->getEnabledCheckBox());
            p.bellows["LUT"] = ftk::Bellows::create(context, "LUT", layout);
            p.bellows["LUT"]->setWidget(lutWidget);
            p.bellows["LUT"]->setToolWidget(lutWidget->getEnabledCheckBox());
#endif // TLRENDER_OCIO
            p.bellows["Color"] = ftk::Bellows::create(context, "Color", layout);
            p.bellows["Color"]->setWidget(colorWidget);
            p.bellows["Color"]->setToolWidget(colorWidget->getEnabledCheckBox());
            p.bellows["Levels"] = ftk::Bellows::create(context, "Levels", layout);
            p.bellows["Levels"]->setWidget(levelsWidget);
            p.bellows["Levels"]->setToolWidget(levelsWidget->getEnabledCheckBox());
            // Reset where the settings being reset are, as well as in the
            // menu: the tool is where somebody looks for it (DJV #687). The
            // same dialog either way.
            p.resetButton = ftk::PushButton::create(context, "Reset");
            p.resetButton->setTooltip(
                "Reset the color settings to their defaults, choosing which "
                "sections to reset.");

            // No scroll area of its own, so the tool stays the height of
            // its sections rather than taking the panel; the panel scrolls.
            auto toolLayout = ftk::VerticalLayout::create(context);
            toolLayout->setSpacingRole(ftk::SizeRole::None);
            layout->setParent(toolLayout);
            ftk::Divider::create(context, ftk::Orientation::Vertical, toolLayout);
            auto hLayout = ftk::HorizontalLayout::create(context, toolLayout);
            hLayout->setMarginRole(ftk::SizeRole::MarginSmall);
            hLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            hLayout->addSpacer(ftk::Stretch::Expanding);
            p.resetButton->setParent(hLayout);
            _setWidget(toolLayout);

            std::weak_ptr<App> appWeak(app);
            p.resetButton->setClickedCallback(
                [appWeak]
                {
                    if (auto app = appWeak.lock())
                    {
                        app->colorResetDialog();
                    }
                });

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
            const std::shared_ptr<MainWindow>& mainWindow,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<ColorTool>(new ColorTool);
            out->_init(context, app, mainWindow, parent);
            return out;
        }
    }
}
