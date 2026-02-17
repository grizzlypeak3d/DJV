// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/SettingsToolPrivate.h>

#include <djv/App/App.h>

#include <ftk/UI/ComboBox.h>
#include <ftk/UI/ColorSwatch.h>
#include <ftk/UI/FloatEditSlider.h>
#include <ftk/UI/FormLayout.h>
#include <ftk/UI/PushButton.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/Core/Format.h>

namespace djv
{
    namespace app
    {
        struct StyleSettingsWidget::Private
        {
            std::shared_ptr<models::SettingsModel> model;

            const std::vector<float> displayScales = ftk::getDisplayScales();

            std::shared_ptr<ftk::ComboBox> colorStyleComboBox;
            std::shared_ptr<ftk::FloatEditSlider> brightnessSlider;
            std::shared_ptr<ftk::FloatEditSlider> contrastSlider;
            std::shared_ptr<ftk::ComboBox> displayScaleComboBox;
            std::shared_ptr<ftk::FormLayout> layout;

            std::shared_ptr<ftk::Observer<models::StyleSettings> > settingsObserver;
        };

        void StyleSettingsWidget::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            ISettingsWidget::_init(context, "djv::app::StyleSettingsWidget", parent);
            FTK_P();

            p.model = app->getSettingsModel();

            p.colorStyleComboBox = ftk::ComboBox::create(context, ftk::getColorStyleLabels());
            p.colorStyleComboBox->setHStretch(ftk::Stretch::Expanding);

            p.brightnessSlider = ftk::FloatEditSlider::create(context);
            p.brightnessSlider->setRange(.5F, 1.5F);
            p.brightnessSlider->setDefaultValue(1.F);

            p.contrastSlider = ftk::FloatEditSlider::create(context);
            p.contrastSlider->setRange(.5F, 1.5F);
            p.contrastSlider->setDefaultValue(1.F);

            std::vector<std::string> labels;
            for (auto d : p.displayScales)
            {
                labels.push_back(ftk::Format("{0}").arg(d).operator std::string());
            }
            p.displayScaleComboBox = ftk::ComboBox::create(context, labels);
            p.displayScaleComboBox->setHStretch(ftk::Stretch::Expanding);

            p.layout = ftk::FormLayout::create(context, shared_from_this());
            p.layout->setMarginRole(ftk::SizeRole::Margin);
            p.layout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.layout->addRow("Color style:", p.colorStyleComboBox);
            p.layout->addRow("Brightness:", p.brightnessSlider);
            p.layout->addRow("Contrast:", p.contrastSlider);
            p.layout->addRow("Display scale:", p.displayScaleComboBox);

            p.settingsObserver = ftk::Observer<models::StyleSettings>::create(
                app->getSettingsModel()->observeStyle(),
                [this](const models::StyleSettings& value)
                {
                    _widgetUpdate(value);
                });

            p.colorStyleComboBox->setIndexCallback(
                [this](int value)
                {
                    FTK_P();
                    auto settings = p.model->getStyle();
                    settings.colorStyle = static_cast<ftk::ColorStyle>(value);
                    p.model->setStyle(settings);
                });

            p.brightnessSlider->setCallback(
                [this](float value)
                {
                    FTK_P();
                    auto settings = p.model->getStyle();
                    settings.colorControls.brightness = value;
                    p.model->setStyle(settings);
                });

            p.contrastSlider->setCallback(
                [this](float value)
                {
                    FTK_P();
                    auto settings = p.model->getStyle();
                    settings.colorControls.contrast = value;
                    p.model->setStyle(settings);
                });

            p.displayScaleComboBox->setIndexCallback(
                [this](int value)
                {
                    FTK_P();
                    auto settings = p.model->getStyle();
                    if (value >= 0 && value < p.displayScales.size())
                    {
                        settings.displayScale = p.displayScales[value];
                    }
                    p.model->setStyle(settings);
                });
        }

        StyleSettingsWidget::StyleSettingsWidget() :
            _p(new Private)
        {}

        StyleSettingsWidget::~StyleSettingsWidget()
        {}

        std::shared_ptr<StyleSettingsWidget> StyleSettingsWidget::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<StyleSettingsWidget>(new StyleSettingsWidget);
            out->_init(context, app, parent);
            return out;
        }

        ftk::Size2I StyleSettingsWidget::getSizeHint() const
        {
            return _p->layout->getSizeHint();
        }

        void StyleSettingsWidget::setGeometry(const ftk::Box2I& value)
        {
            ISettingsWidget::setGeometry(value);
            _p->layout->setGeometry(value);
        }

        void StyleSettingsWidget::_widgetUpdate(const models::StyleSettings& value)
        {
            FTK_P();

            p.colorStyleComboBox->setCurrentIndex(static_cast<int>(value.colorStyle));

            p.brightnessSlider->setValue(value.colorControls.brightness);
            p.contrastSlider->setValue(value.colorControls.contrast);

            const auto i = std::find(
                p.displayScales.begin(),
                p.displayScales.end(),
                value.displayScale);
            p.displayScaleComboBox->setCurrentIndex(
                i != p.displayScales.end() ?
                (i - p.displayScales.begin()) :
                -1);
        }
    }
}
