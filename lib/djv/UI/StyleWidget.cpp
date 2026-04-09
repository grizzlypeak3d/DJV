// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UI/SettingsWidgets.h>

#include <ftk/UI/ComboBox.h>
#include <ftk/UI/ColorSwatch.h>
#include <ftk/UI/FileEdit.h>
#include <ftk/UI/FloatEditSlider.h>
#include <ftk/UI/FormLayout.h>
#include <ftk/UI/PushButton.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/Core/Format.h>

namespace djv
{
    namespace ui
    {
        struct StyleSettingsWidget::Private
        {
            std::shared_ptr<models::SettingsModel> settings;
            std::vector<std::string> fonts;

            const std::vector<float> displayScales = ftk::getDisplayScales();

            std::shared_ptr<ftk::ComboBox> colorStyleComboBox;
            std::shared_ptr<ftk::FloatEditSlider> brightnessSlider;
            std::shared_ptr<ftk::FloatEditSlider> contrastSlider;
            std::shared_ptr<ftk::ComboBox> displayScaleComboBox;
            std::map<ftk::FontType, std::shared_ptr<ftk::ComboBox> > fontComboBoxes;
            std::vector<std::shared_ptr<ftk::FileEdit> > fontFileEdits;
            std::shared_ptr<ftk::FormLayout> layout;

            std::shared_ptr<ftk::ListObserver<std::string> > fontsObserver;
            std::shared_ptr<ftk::Observer<models::StyleSettings> > styleObserver;
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

            std::vector<std::string> labels;
            for (auto d : p.displayScales)
            {
                labels.push_back(ftk::Format("{0}").arg(d).operator std::string());
            }
            p.displayScaleComboBox = ftk::ComboBox::create(context, labels);
            p.displayScaleComboBox->setHStretch(ftk::Stretch::Expanding);

            const auto fontLabels = ftk::getFontTypeLabels();
            for (const auto font : ftk::getFontTypeEnums())
            {
                p.fontComboBoxes[font] = ftk::ComboBox::create(context);
            }
            for (size_t i = 0; i < 4; ++i)
            {
                p.fontFileEdits.push_back(ftk::FileEdit::create(context));
            }

            p.layout = ftk::FormLayout::create(context, shared_from_this());
            p.layout->setMarginRole(ftk::SizeRole::Margin);
            p.layout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.layout->addRow("Color style:", p.colorStyleComboBox);
            p.layout->addRow("Brightness:", p.brightnessSlider);
            p.layout->addRow("Contrast:", p.contrastSlider);
            p.layout->addRow("Display scale:", p.displayScaleComboBox);
            for (const auto font : ftk::getFontTypeEnums())
            {
                p.layout->addRow(ftk::Format("{0} font:").arg(
                    fontLabels[static_cast<int>(font)]), p.fontComboBoxes[font]);
            }
            for (size_t i = 0; i < p.fontFileEdits.size(); ++i)
            {
                p.layout->addRow("Font file:", p.fontFileEdits[i]);
            }

            auto fontSystem = context->getSystem<ftk::FontSystem>();
            p.fontsObserver = ftk:: ListObserver<std::string>::create(
                fontSystem->observeFonts(),
                [this](const std::vector<std::string>& value)
                {
                    FTK_P();
                    p.fonts = value;
                    for (const auto font : ftk::getFontTypeEnums())
                    {
                        p.fontComboBoxes[font]->setItems(value);
                    }
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
                    if (value >= 0 && value < p.displayScales.size())
                    {
                        settings.displayScale = p.displayScales[value];
                    }
                    p.settings->setStyle(settings);
                });

            for (const auto font : ftk::getFontTypeEnums())
            {
                p.fontComboBoxes[font]->setIndexCallback(
                    [this, font](int index)
                    {
                        FTK_P();
                        auto settings = p.settings->getStyle();
                        settings.fonts[font] = p.fonts[index];
                        p.settings->setStyle(settings);
                    });
            }

            for (int i = 0; i < p.fontFileEdits.size(); ++i)
            {
                p.fontFileEdits[i]->setCallback(
                    [this, i](const ftk::Path& value)
                    {
                        FTK_P();
                        auto settings = p.settings->getStyle();
                        settings.fontFiles.resize(i + 1);
                        settings.fontFiles[i] = value.get();
                        p.settings->setStyle(settings);
                    });
            }
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

            for (const auto i : value.fonts)
            {
                const auto j = std::find(p.fonts.begin(), p.fonts.end(), i.second);
                p.fontComboBoxes[i.first]->setCurrentIndex(j != p.fonts.end() ? (j - p.fonts.begin()) : -1);
            }
            for (size_t i = 0; i < p.fontFileEdits.size(); ++i)
            {
                p.fontFileEdits[i]->setPath(i < value.fontFiles.size() ?
                    ftk::Path(value.fontFiles[i]) :
                    ftk::Path());
            }
        }
    }
}
