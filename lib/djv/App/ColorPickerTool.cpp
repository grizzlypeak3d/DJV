// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/ColorPickerTool.h>

#include <djv/App/App.h>
#include <djv/App/MainWindow.h>
#include <djv/UI/Viewport.h>
#include <djv/Models/SettingsModel.h>
#include <djv/Models/ViewportModel.h>

#include <ftk/UI/ColorSwatch.h>
#include <ftk/UI/FormLayout.h>
#include <ftk/UI/Label.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/ScreenshotTag.h>
#include <ftk/Core/Format.h>

namespace djv
{
    namespace app
    {
        struct ColorPickerTool::Private
        {
            std::shared_ptr<ftk::ColorSwatch> colorSwatch;
            std::weak_ptr<ui::Viewport> viewport;
            std::optional<ftk::Color4F> colorSample;

            std::shared_ptr<ftk::Label> colorLabel;
            std::shared_ptr<ftk::Label> luminanceLabel;
            std::shared_ptr<ftk::Label> pixelLabel;
            std::shared_ptr<ftk::Label> mouseLabel;

            std::shared_ptr<ftk::Observer<std::optional<ftk::V2I> > > pickObserver;
            std::shared_ptr<ftk::Observer<std::optional<ftk::Color4F> > > colorSampleObserver;
            std::shared_ptr<ftk::Observer<tl::HDR_EOTF> > hdrTransferObserver;
            std::shared_ptr<ftk::Observer<float> > hdrWhiteObserver;
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
                mainWindow,
                "Color Picker",
                "ColorPicker",
                "djv::app::ColorPickerTool",
                parent);
            FTK_P();

            p.viewport = mainWindow->getViewport();

            p.colorSwatch = ftk::ColorSwatch::create(context);
            p.colorSwatch->setColor(ftk::Color4F(0.F, 0.F, 0.F));
            p.colorSwatch->setBorder(false);
            p.colorSwatch->setSizeRole(ftk::SizeRole::SwatchLarge);

            p.colorLabel = ftk::Label::create(context);
            p.colorLabel->setFont(ftk::FontType::Mono);
            ftk::setScreenshotTag(p.colorLabel, "ColorPicker.Color");

            p.luminanceLabel = ftk::Label::create(context);
            p.luminanceLabel->setFont(ftk::FontType::Mono);
            p.luminanceLabel->setTooltip(
                "The luminance the color stands for: what a PQ picture's "
                "code values say, or what an HDR window makes of an SDR "
                "picture. See HDR picture in the View tool.");
            ftk::setScreenshotTag(p.luminanceLabel, "ColorPicker.Luminance");

            p.pixelLabel = ftk::Label::create(context);
            p.pixelLabel->setFont(ftk::FontType::Mono);
            ftk::setScreenshotTag(p.pixelLabel, "ColorPicker.Pixel");

            p.mouseLabel = ftk::Label::create(context);
            ftk::setScreenshotTag(p.mouseLabel, "ColorPicker.Mouse");

            auto layout = ftk::VerticalLayout::create(context);
            layout->setSpacingRole(ftk::SizeRole::None);
            p.colorSwatch->setParent(layout);
            auto formLayout = ftk::FormLayout::create(context, layout);
            formLayout->setMarginRole(ftk::SizeRole::Margin);
            formLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            formLayout->addRow("Color:", p.colorLabel);
            formLayout->addRow("Luminance:", p.luminanceLabel);
            formLayout->addRow("Pixel:", p.pixelLabel);
            formLayout->addRow("Mouse:", p.mouseLabel);

            _setWidget(layout);

            p.pickObserver = ftk::Observer<std::optional<ftk::V2I> >::create(
                mainWindow->getViewport()->observePick(),
                [this](const std::optional<ftk::V2I>& value)
                {
                    std::string text = "-";
                    if (value.has_value())
                    {
                        text = ftk::Format("{0}").arg(value.value());
                    }
                    _p->pixelLabel->setText(text);
                });

            p.colorSampleObserver = ftk::Observer<std::optional<ftk::Color4F> >::create(
                mainWindow->getViewport()->observeColorSample(),
                [this](const std::optional<ftk::Color4F>& value)
                {
                    _p->colorSample = value;
                    _colorUpdate();
                });

            // What a color stands for, and is shown as, turns on what the
            // picture is said to be as well as on the color.
            p.hdrTransferObserver = ftk::Observer<tl::HDR_EOTF>::create(
                app->getViewportModel()->observeHDRTransfer(),
                [this](tl::HDR_EOTF)
                {
                    _colorUpdate();
                });

            p.hdrWhiteObserver = ftk::Observer<float>::create(
                app->getViewportModel()->observeHDRWhite(),
                [this](float)
                {
                    _colorUpdate();
                });

            p.settingsObserver = ftk::Observer<models::MouseSettings>::create(
                app->getSettingsModel()->observeMouse(),
                [this](const models::MouseSettings& value)
                {
                    std::vector<std::string> s;
                    if (auto i = value.bindings.find(models::MouseAction::Pick);
                        i != value.bindings.end())
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

        void ColorPickerTool::_colorUpdate()
        {
            FTK_P();
            // The widgets are made before the first observer speaks.
            if (!p.luminanceLabel)
                return;
            ftk::Color4F swatch;
            std::string colorText = "-";
            std::string luminanceText = "-";
            if (p.colorSample.has_value())
            {
                const ftk::Color4F& color = p.colorSample.value();
                swatch = color;
                colorText = ftk::Format("{0} {1} {2} {3}").
                    arg(color.r, 2).
                    arg(color.g, 2).
                    arg(color.b, 2).
                    arg(color.a, 2);
                if (auto viewport = p.viewport.lock())
                {
                    // The swatch is the color as the picture shows it.
                    swatch = viewport->getColorSampleDisplay(color);
                    if (const auto nits = viewport->getColorSampleNits(color))
                    {
                        luminanceText = ftk::Format("{0} nits").arg(nits.value(), 2);
                    }
                }
            }
            p.colorSwatch->setColor(swatch);
            p.colorLabel->setText(colorText);
            p.luminanceLabel->setText(luminanceText);
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
