// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UI/SettingsWidgets.h>

#include <ftk/UI/CheckBox.h>
#include <ftk/UI/ComboBox.h>
#include <ftk/UI/ColorSwatch.h>
#include <ftk/UI/FloatEditSlider.h>
#include <ftk/UI/FormLayout.h>
#include <ftk/UI/PushButton.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/ScreenshotTag.h>
#include <ftk/Core/Format.h>

namespace djv
{
    namespace ui
    {
        struct StyleSettingsWidget::Private
        {
            std::shared_ptr<models::SettingsModel> settings;

            const std::vector<float> displayScales = ftk::getDisplayScales();

            std::shared_ptr<ftk::ComboBox> colorStyleComboBox;
            std::shared_ptr<ftk::FloatEditSlider> brightnessSlider;
            std::shared_ptr<ftk::FloatEditSlider> contrastSlider;
            std::shared_ptr<ftk::ComboBox> displayScaleComboBox;
            std::shared_ptr<ftk::CheckBox> tooltipsCheckBox;
            std::shared_ptr<ftk::FormLayout> layout;

            std::shared_ptr<ftk::Observer<models::StyleSettings> > styleObserver;
            std::shared_ptr<ftk::Observer<models::MiscSettings> > miscObserver;
        };

        void StyleSettingsWidget::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::SettingsModel>& settings,
            const std::shared_ptr<IWidget>& parent)
        {
            ISettingsWidget::_init(context, "djv::ui::StyleSettingsWidget", parent);
            FTK_P();

            p.settings = settings;

            p.colorStyleComboBox = ftk::ComboBox::create(context, ftk::getColorStyleLabels());
            p.colorStyleComboBox->setHStretch(ftk::Stretch::Expanding);

            p.brightnessSlider = ftk::FloatEditSlider::create(context);
            p.brightnessSlider->setRange(.5F, 1.5F);
            p.brightnessSlider->setDefault(1.F);

            p.contrastSlider = ftk::FloatEditSlider::create(context);
            p.contrastSlider->setRange(.5F, 1.5F);
            p.contrastSlider->setDefault(1.F);

            // The first entry is automatic: zero in the setting.
            std::vector<std::string> labels = { "Auto" };
            for (auto d : p.displayScales)
            {
                labels.push_back(ftk::Format("{0}").arg(d).operator std::string());
            }
            p.displayScaleComboBox = ftk::ComboBox::create(context, labels);
            p.displayScaleComboBox->setHStretch(ftk::Stretch::Expanding);


            // Whether the interface explains itself is of a piece with how
            // it looks; it was the last thing in a section called
            // "Miscellaneous", which is where a setting goes to hide
            // (DJV #899). The setting is stored where it always was.
            p.tooltipsCheckBox = ftk::CheckBox::create(context);
            p.tooltipsCheckBox->setHStretch(ftk::Stretch::Expanding);

            p.layout = ftk::FormLayout::create(context);

            _setWidget(p.layout);
            p.layout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.layout->addRow("Color style:", p.colorStyleComboBox);
            p.layout->addRow("Brightness:", p.brightnessSlider);
            p.layout->addRow("Contrast:", p.contrastSlider);
            p.layout->addRow("Display scale:", p.displayScaleComboBox);
            p.layout->addRow("Enable tooltips:", p.tooltipsCheckBox);

            p.miscObserver = ftk::Observer<models::MiscSettings>::create(
                settings->observeMisc(),
                [this](const models::MiscSettings& value)
                {
                    _p->tooltipsCheckBox->setChecked(value.tooltipsEnabled);
                });

            p.tooltipsCheckBox->setCheckedCallback(
                [this](bool value)
                {
                    FTK_P();
                    auto settings = p.settings->getMisc();
                    settings.tooltipsEnabled = value;
                    p.settings->setMisc(settings);
                });

            p.styleObserver = ftk::Observer<models::StyleSettings>::create(
                settings->observeStyle(),
                [this](const models::StyleSettings& value)
                {
                    _widgetUpdate(value);
                });

            p.colorStyleComboBox->setIndexCallback(
                [this](int value)
                {
                    FTK_P();
                    auto settings = p.settings->getStyle();
                    settings.colorStyle = static_cast<ftk::ColorStyle>(value);
                    p.settings->setStyle(settings);
                });

            p.brightnessSlider->setCallback(
                [this](float value)
                {
                    FTK_P();
                    auto settings = p.settings->getStyle();
                    settings.colorControls.brightness = value;
                    p.settings->setStyle(settings);
                });

            p.contrastSlider->setCallback(
                [this](float value)
                {
                    FTK_P();
                    auto settings = p.settings->getStyle();
                    settings.colorControls.contrast = value;
                    p.settings->setStyle(settings);
                });

            p.displayScaleComboBox->setIndexCallback(
                [this](int value)
                {
                    FTK_P();
                    auto settings = p.settings->getStyle();
                    if (0 == value)
                    {
                        settings.displayScale = 0.F;
                    }
                    else if (value > 0 && value <= static_cast<int>(p.displayScales.size()))
                    {
                        settings.displayScale = p.displayScales[value - 1];
                    }
                    p.settings->setStyle(settings);
                });

        }

        StyleSettingsWidget::StyleSettingsWidget() :
            _p(new Private)
        {}

        StyleSettingsWidget::~StyleSettingsWidget()
        {}

        std::shared_ptr<StyleSettingsWidget> StyleSettingsWidget::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::SettingsModel>& settings,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<StyleSettingsWidget>(new StyleSettingsWidget);
            out->_init(context, settings, parent);
            return out;
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
                value.displayScale <= 0.F ?
                0 :
                i != p.displayScales.end() ?
                (i - p.displayScales.begin()) + 1 :
                -1);

        }
    }
}
