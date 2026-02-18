// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/ColorPickerTool.h>

#include <djv/App/App.h>
#include <djv/App/MainWindow.h>
#include <djv/App/Viewport.h>
#include <djv/Models/SettingsModel.h>

#include <ftk/UI/ColorSwatch.h>
#include <ftk/UI/FormLayout.h>
#include <ftk/UI/Label.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/ScrollWidget.h>
#include <ftk/Core/Format.h>

namespace djv
{
    namespace app
    {
        struct ColorPickerTool::Private
        {
            std::shared_ptr<ftk::ColorSwatch> colorSwatch;
            std::shared_ptr<ftk::Label> colorLabel;
            std::shared_ptr<ftk::Label> pixelLabel;
            std::shared_ptr<ftk::Label> mouseLabel;

            std::shared_ptr<ftk::Observer<ftk::V2I> > pickObserver;
            std::shared_ptr<ftk::Observer<ftk::Color4F> > colorSampleObserver;
            std::shared_ptr<ftk::Observer<models::MouseSettings> > settingsObserver;
        };

        void ColorPickerTool::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<MainWindow>& mainWindow,
            const std::shared_ptr<IWidget>& parent)
        {
            IToolWidget::_init(
                context,
                app,
                models::Tool::ColorPicker,
                "djv::app::ColorPickerTool",
                parent);
            FTK_P();

            p.colorSwatch = ftk::ColorSwatch::create(context);
            p.colorSwatch->setColor(ftk::Color4F(0.F, 0.F, 0.F));
            p.colorSwatch->setSizeRole(ftk::SizeRole::SwatchLarge);

            p.colorLabel = ftk::Label::create(context);
            p.colorLabel->setFontRole(ftk::FontRole::Mono);

            p.pixelLabel = ftk::Label::create(context);
            p.pixelLabel->setFontRole(ftk::FontRole::Mono);

            p.mouseLabel = ftk::Label::create(context);

            auto layout = ftk::VerticalLayout::create(context);
            layout->setMarginRole(ftk::SizeRole::Margin);
            p.colorSwatch->setParent(layout);
            auto formLayout = ftk::FormLayout::create(context, layout);
            formLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            formLayout->addRow("Color:", p.colorLabel);
            formLayout->addRow("Pixel:", p.pixelLabel);
            formLayout->addRow("Mouse:", p.mouseLabel);

            auto scrollWidget = ftk::ScrollWidget::create(context);
            scrollWidget->setBorder(false);
            scrollWidget->setWidget(layout);
            _setWidget(scrollWidget);

            p.pickObserver = ftk::Observer<ftk::V2I>::create(
                mainWindow->getViewport()->observePick(),
                [this](const ftk::V2I& value)
                {
                    _p->pixelLabel->setText(ftk::Format("{0}").arg(value));
                });

            p.colorSampleObserver = ftk::Observer<ftk::Color4F>::create(
                mainWindow->getViewport()->observeColorSample(),
                [this](const ftk::Color4F& value)
                {
                    FTK_P();
                    p.colorSwatch->setColor(value);
                    p.colorLabel->setText(ftk::Format("{0} {1} {2} {3}").
                        arg(value.r, 2).
                        arg(value.g, 2).
                        arg(value.b, 2).
                        arg(value.a, 2));
                });

            p.settingsObserver = ftk::Observer<models::MouseSettings>::create(
                app->getSettingsModel()->observeMouse(),
                [this](const models::MouseSettings& value)
                {
                    std::vector<std::string> s;
                    auto i = value.bindings.find(models::MouseAction::Pick);
                    if (i != value.bindings.end())
                    {
                        if (i->second.button != ftk::MouseButton::None)
                        {
                            if (i->second.modifier != ftk::KeyModifier::None)
                            {
                                s.push_back(ftk::to_string(i->second.modifier));
                            }
                            s.push_back(ftk::getLabel(i->second.button));
                        }
                    }
                    _p->mouseLabel->setText(
                        ftk::Format("{0} Click").arg(ftk::join(s, " + ")));
                });
        }

        ColorPickerTool::ColorPickerTool() :
            _p(new Private)
        {}

        ColorPickerTool::~ColorPickerTool()
        {}

        std::shared_ptr<ColorPickerTool> ColorPickerTool::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<MainWindow>& mainWindow,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<ColorPickerTool>(new ColorPickerTool);
            out->_init(context, app, mainWindow, parent);
            return out;
        }
    }
}
