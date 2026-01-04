// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djvApp/Tools/ColorToolPrivate.h>

#include <djvApp/Models/ColorModel.h>
#include <djvApp/Models/ViewportModel.h>
#include <djvApp/App.h>

#include <ftk/UI/Bellows.h>
#include <ftk/UI/ButtonGroup.h>
#include <ftk/UI/CheckBox.h>
#include <ftk/UI/ComboBox.h>
#include <ftk/UI/FileEdit.h>
#include <ftk/UI/FloatEdit.h>
#include <ftk/UI/FloatEditSlider.h>
#include <ftk/UI/FormLayout.h>
#include <ftk/UI/Label.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/ScrollWidget.h>
#include <ftk/UI/Settings.h>
#include <ftk/UI/StackLayout.h>

namespace djv
{
    namespace app
    {
        struct OCIOWidget::Private
        {
            std::shared_ptr<OCIOModel> ocioModel;

            std::shared_ptr<ftk::CheckBox> enabledCheckBox;
            std::shared_ptr<ftk::ComboBox> configComboBox;
            std::shared_ptr<ftk::FileEdit> fileEdit;
            std::shared_ptr<ftk::Label> nameLabel;
            std::shared_ptr<ftk::ComboBox> inputComboBox;
            std::shared_ptr<ftk::ComboBox> displayComboBox;
            std::shared_ptr<ftk::ComboBox> viewComboBox;
            std::shared_ptr<ftk::ComboBox> lookComboBox;
            std::shared_ptr<ftk::FormLayout> formLayout;
            std::shared_ptr<ftk::VerticalLayout> layout;

            std::shared_ptr<ftk::Observer<tl::timeline::OCIOOptions> > optionsObserver;
            std::shared_ptr<ftk::Observer<tl::timeline::OCIOOptions> > optionsObserver2;
            std::shared_ptr<ftk::Observer<OCIOModelData> > dataObserver;
        };

        void OCIOWidget::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<ftk::IWidget>& parent)
        {
            ftk::IWidget::_init(context, "djv::app::OCIOWidget", parent);
            FTK_P();
            
#if !defined(TLRENDER_OCIO)
            setEnabled(false);
#endif // TLRENDER_OCIO

            p.ocioModel = OCIOModel::create(context);

            p.configComboBox = ftk::ComboBox::create(context, tl::timeline::getOCIOConfigLabels());
            p.configComboBox->setHStretch(ftk::Stretch::Expanding);

            p.enabledCheckBox = ftk::CheckBox::create(context);

            p.fileEdit = ftk::FileEdit::create(context);

            p.nameLabel = ftk::Label::create(context);

            p.inputComboBox = ftk::ComboBox::create(context);
            p.inputComboBox->setHStretch(ftk::Stretch::Expanding);

            p.displayComboBox = ftk::ComboBox::create(context);
            p.displayComboBox->setHStretch(ftk::Stretch::Expanding);

            p.viewComboBox = ftk::ComboBox::create(context);
            p.viewComboBox->setHStretch(ftk::Stretch::Expanding);

            p.lookComboBox = ftk::ComboBox::create(context);
            p.lookComboBox->setHStretch(ftk::Stretch::Expanding);

            p.layout = ftk::VerticalLayout::create(context, shared_from_this());
            p.layout->setMarginRole(ftk::SizeRole::Margin);
            p.layout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.formLayout = ftk::FormLayout::create(context, p.layout);
            p.formLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.formLayout->addRow("Enabled:", p.enabledCheckBox);
            p.formLayout->addRow("Configuration:", p.configComboBox);
            p.formLayout->addRow("File name:", p.fileEdit);
            p.formLayout->addRow("Name:", p.nameLabel);
            p.formLayout->addRow("Input:", p.inputComboBox);
            p.formLayout->addRow("Display:", p.displayComboBox);
            p.formLayout->addRow("View:", p.viewComboBox);
            p.formLayout->addRow("Look:", p.lookComboBox);

            p.optionsObserver = ftk::Observer<tl::timeline::OCIOOptions>::create(
                app->getColorModel()->observeOCIOOptions(),
                [this](const tl::timeline::OCIOOptions& value)
                {
                    _p->ocioModel->setOptions(value);
                });

            auto appWeak = std::weak_ptr<App>(app);
            p.optionsObserver2 = ftk::Observer<tl::timeline::OCIOOptions>::create(
                p.ocioModel->observeOptions(),
                [appWeak](const tl::timeline::OCIOOptions& value)
                {
                    if (auto app = appWeak.lock())
                    {
                        app->getColorModel()->setOCIOOptions(value);
                    }
                });

            p.dataObserver = ftk::Observer<OCIOModelData>::create(
                p.ocioModel->observeData(),
                [this](const OCIOModelData& value)
                {
                    FTK_P();
                    p.enabledCheckBox->setChecked(value.enabled);
                    p.configComboBox->setCurrentIndex(static_cast<int>(value.config));
                    p.fileEdit->setPath(ftk::Path(value.fileName));
                    p.nameLabel->setText(value.name);
                    p.formLayout->setRowVisible(p.fileEdit, tl::timeline::OCIOConfig::File == value.config);
                    p.inputComboBox->setItems(value.inputs);
                    p.inputComboBox->setCurrentIndex(value.inputIndex);
                    p.displayComboBox->setItems(value.displays);
                    p.displayComboBox->setCurrentIndex(value.displayIndex);
                    p.viewComboBox->setItems(value.views);
                    p.viewComboBox->setCurrentIndex(value.viewIndex);
                    p.lookComboBox->setItems(value.looks);
                    p.lookComboBox->setCurrentIndex(value.lookIndex);
                });

            p.enabledCheckBox->setCheckedCallback(
                [this](bool value)
                {
                    _p->ocioModel->setEnabled(value);
                });

            p.configComboBox->setIndexCallback(
                [this](int value)
                {
                    _p->ocioModel->setConfig(static_cast<tl::timeline::OCIOConfig>(value));
                });

            p.fileEdit->setCallback(
                [this](const ftk::Path& value)
                {
                    _p->ocioModel->setFileName(value.get());
                });

            p.inputComboBox->setIndexCallback(
                [this](int index)
                {
                    _p->ocioModel->setInputIndex(index);
                });
            p.displayComboBox->setIndexCallback(
                [this](int index)
                {
                    _p->ocioModel->setDisplayIndex(index);
                });
            p.viewComboBox->setIndexCallback(
                [this](int index)
                {
                    _p->ocioModel->setViewIndex(index);
                });
            p.lookComboBox->setIndexCallback(
                [this](int index)
                {
                    _p->ocioModel->setLookIndex(index);
                });
        }

        OCIOWidget::OCIOWidget() :
            _p(new Private)
        {}

        OCIOWidget::~OCIOWidget()
        {}

        std::shared_ptr<OCIOWidget> OCIOWidget::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<OCIOWidget>(new OCIOWidget);
            out->_init(context, app, parent);
            return out;
        }

        ftk::Size2I OCIOWidget::getSizeHint() const
        {
            return _p->layout->getSizeHint();
        }

        void OCIOWidget::setGeometry(const ftk::Box2I& value)
        {
            IWidget::setGeometry(value);
            _p->layout->setGeometry(value);
        }

        struct LUTWidget::Private
        {
            std::shared_ptr<ftk::CheckBox> enabledCheckBox;
            std::shared_ptr<ftk::FileEdit> fileEdit;
            std::shared_ptr<ftk::ComboBox> orderComboBox;
            std::shared_ptr<ftk::FormLayout> layout;

            std::shared_ptr<ftk::Observer<tl::timeline::LUTOptions> > optionsObservers;
        };

        void LUTWidget::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<ftk::IWidget>& parent)
        {
            ftk::IWidget::_init(context, "djv::app::LUTWidget", parent);
            FTK_P();
            
#if !defined(TLRENDER_OCIO)
            setEnabled(false);
#endif // TLRENDER_OCIO

            p.enabledCheckBox = ftk::CheckBox::create(context);

            p.fileEdit = ftk::FileEdit::create(context);

            p.orderComboBox = ftk::ComboBox::create(context, tl::timeline::getLUTOrderLabels());
            p.orderComboBox->setHStretch(ftk::Stretch::Expanding);

            p.layout = ftk::FormLayout::create(context, shared_from_this());
            p.layout->setMarginRole(ftk::SizeRole::Margin);
            p.layout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.layout->addRow("Enabled:", p.enabledCheckBox);
            p.layout->addRow("File name:", p.fileEdit);
            p.layout->addRow("Order:", p.orderComboBox);

            p.optionsObservers = ftk::Observer<tl::timeline::LUTOptions>::create(
                app->getColorModel()->observeLUTOptions(),
                [this](const tl::timeline::LUTOptions& value)
                {
                    _p->enabledCheckBox->setChecked(value.enabled);
                    _p->fileEdit->setPath(ftk::Path(value.fileName));
                    _p->orderComboBox->setCurrentIndex(static_cast<size_t>(value.order));
                });

            auto appWeak = std::weak_ptr<App>(app);
            p.enabledCheckBox->setCheckedCallback(
                [appWeak](bool value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getColorModel()->getLUTOptions();
                        options.enabled = value;
                        app->getColorModel()->setLUTOptions(options);
                    }
                });

            p.fileEdit->setCallback(
                [appWeak](const ftk::Path& value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getColorModel()->getLUTOptions();
                        options.enabled = true;
                        options.fileName = value.get();
                        app->getColorModel()->setLUTOptions(options);
                    }
                });

            p.orderComboBox->setIndexCallback(
                [appWeak](int value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getColorModel()->getLUTOptions();
                        options.enabled = true;
                        options.order = static_cast<tl::timeline::LUTOrder>(value);
                        app->getColorModel()->setLUTOptions(options);
                    }
                });
        }

        LUTWidget::LUTWidget() :
            _p(new Private)
        {}

        LUTWidget::~LUTWidget()
        {}

        std::shared_ptr<LUTWidget> LUTWidget::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<LUTWidget>(new LUTWidget);
            out->_init(context, app, parent);
            return out;
        }

        ftk::Size2I LUTWidget::getSizeHint() const
        {
            return _p->layout->getSizeHint();
        }

        void LUTWidget::setGeometry(const ftk::Box2I& value)
        {
            IWidget::setGeometry(value);
            _p->layout->setGeometry(value);
        }

        struct ColorWidget::Private
        {
            std::shared_ptr<ftk::CheckBox> enabledCheckBox;
            std::map<std::string, std::shared_ptr<ftk::FloatEditSlider> > sliders;
            std::shared_ptr<ftk::CheckBox> invertCheckBox;
            std::shared_ptr<ftk::FormLayout> layout;

            std::shared_ptr<ftk::Observer<tl::timeline::DisplayOptions> > optionsObservers;
        };

        void ColorWidget::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<ftk::IWidget>& parent)
        {
            ftk::IWidget::_init(context, "djv::app::ColorWidget", parent);
            FTK_P();

            p.enabledCheckBox = ftk::CheckBox::create(context);

            p.sliders["Add"] = ftk::FloatEditSlider::create(context);
            p.sliders["Add"]->setRange(-1.F, 1.F);
            p.sliders["Add"]->setDefaultValue(0.F);
            p.sliders["Brightness"] = ftk::FloatEditSlider::create(context);
            p.sliders["Brightness"]->setRange(0.F, 4.F);
            p.sliders["Brightness"]->setDefaultValue(1.F);
            p.sliders["Contrast"] = ftk::FloatEditSlider::create(context);
            p.sliders["Contrast"]->setRange(0.F, 4.F);
            p.sliders["Contrast"]->setDefaultValue(1.F);
            p.sliders["Saturation"] = ftk::FloatEditSlider::create(context);
            p.sliders["Saturation"]->setRange(0.F, 4.F);
            p.sliders["Saturation"]->setDefaultValue(1.F);
            p.sliders["Tint"] = ftk::FloatEditSlider::create(context);
            p.sliders["Tint"]->setDefaultValue(1.F);

            p.invertCheckBox = ftk::CheckBox::create(context);

            p.layout = ftk::FormLayout::create(context, shared_from_this());
            p.layout->setMarginRole(ftk::SizeRole::Margin);
            p.layout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.layout->addRow("Enabled:", p.enabledCheckBox);
            p.layout->addRow("Add:", p.sliders["Add"]);
            p.layout->addRow("Brightness:", p.sliders["Brightness"]);
            p.layout->addRow("Contrast:", p.sliders["Contrast"]);
            p.layout->addRow("Saturation:", p.sliders["Saturation"]);
            p.layout->addRow("Tint:", p.sliders["Tint"]);
            p.layout->addRow("Invert:", p.invertCheckBox);

            p.optionsObservers = ftk::Observer<tl::timeline::DisplayOptions>::create(
                app->getViewportModel()->observeDisplayOptions(),
                [this](const tl::timeline::DisplayOptions& value)
                {
                    _p->enabledCheckBox->setChecked(value.color.enabled);
                    _p->sliders["Add"]->setValue(value.color.add.x);
                    _p->sliders["Brightness"]->setValue(value.color.brightness.x);
                    _p->sliders["Contrast"]->setValue(value.color.contrast.x);
                    _p->sliders["Saturation"]->setValue(value.color.saturation.x);
                    _p->sliders["Tint"]->setValue(value.color.tint);
                    _p->invertCheckBox->setChecked(value.color.invert);
                });

            auto appWeak = std::weak_ptr<App>(app);
            p.enabledCheckBox->setCheckedCallback(
                [appWeak](bool value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getViewportModel()->getDisplayOptions();
                        options.color.enabled = value;
                        app->getViewportModel()->setDisplayOptions(options);
                    }
                });

            p.sliders["Add"]->setCallback(
                [appWeak](float value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getViewportModel()->getDisplayOptions();
                        options.color.enabled = true;
                        options.color.add.x = value;
                        options.color.add.y = value;
                        options.color.add.z = value;
                        app->getViewportModel()->setDisplayOptions(options);
                    }
                });

            p.sliders["Brightness"]->setCallback(
                [appWeak](float value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getViewportModel()->getDisplayOptions();
                        options.color.enabled = true;
                        options.color.brightness.x = value;
                        options.color.brightness.y = value;
                        options.color.brightness.z = value;
                        app->getViewportModel()->setDisplayOptions(options);
                    }
                });

            p.sliders["Contrast"]->setCallback(
                [appWeak](float value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getViewportModel()->getDisplayOptions();
                        options.color.enabled = true;
                        options.color.contrast.x = value;
                        options.color.contrast.y = value;
                        options.color.contrast.z = value;
                        app->getViewportModel()->setDisplayOptions(options);
                    }
                });

            p.sliders["Saturation"]->setCallback(
                [appWeak](float value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getViewportModel()->getDisplayOptions();
                        options.color.enabled = true;
                        options.color.saturation.x = value;
                        options.color.saturation.y = value;
                        options.color.saturation.z = value;
                        app->getViewportModel()->setDisplayOptions(options);
                    }
                });

            p.sliders["Tint"]->setCallback(
                [appWeak](float value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getViewportModel()->getDisplayOptions();
                        options.color.enabled = true;
                        options.color.tint = value;
                        app->getViewportModel()->setDisplayOptions(options);
                    }
                });

            p.invertCheckBox->setCheckedCallback(
                [appWeak](bool value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getViewportModel()->getDisplayOptions();
                        options.color.enabled = true;
                        options.color.invert = value;
                        app->getViewportModel()->setDisplayOptions(options);
                    }
                });
        }

        ColorWidget::ColorWidget() :
            _p(new Private)
        {}

        ColorWidget::~ColorWidget()
        {}

        std::shared_ptr<ColorWidget> ColorWidget::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<ColorWidget>(new ColorWidget);
            out->_init(context, app, parent);
            return out;
        }

        ftk::Size2I ColorWidget::getSizeHint() const
        {
            return _p->layout->getSizeHint();
        }

        void ColorWidget::setGeometry(const ftk::Box2I& value)
        {
            IWidget::setGeometry(value);
            _p->layout->setGeometry(value);
        }

        struct LevelsWidget::Private
        {
            std::shared_ptr<ftk::Settings> settings;

            std::shared_ptr<ftk::CheckBox> enabledCheckBox;
            std::map<std::string, std::shared_ptr<ftk::FloatEditSlider> > sliders;
            std::map<std::string, std::shared_ptr<ftk::FloatEdit> > rangeEdits;
            std::shared_ptr<ftk::FormLayout> layout;

            std::shared_ptr<ftk::Observer<tl::timeline::DisplayOptions> > optionsObservers;
        };

        void LevelsWidget::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<ftk::IWidget>& parent)
        {
            ftk::IWidget::_init(context, "djv::app::LevelsWidget", parent);
            FTK_P();

            p.settings = app->getSettings();

            p.enabledCheckBox = ftk::CheckBox::create(context);

            ftk::RangeF range(0.F, 1.F);
            p.settings->getT("/Color/Levels/InRange", range);
            p.sliders["InLow"] = ftk::FloatEditSlider::create(context);
            p.sliders["InLow"]->setRange(range);
            p.sliders["InLow"]->setDefaultValue(0.F);

            p.sliders["InHigh"] = ftk::FloatEditSlider::create(context);
            p.sliders["InHigh"]->setRange(range);
            p.sliders["InHigh"]->setDefaultValue(1.F);

            p.rangeEdits["InMin"] = ftk::FloatEdit::create(context);
            p.rangeEdits["InMin"]->setRange(-1000000.F, 1000000.F);
            p.rangeEdits["InMin"]->setValue(range.min());

            p.rangeEdits["InMax"] = ftk::FloatEdit::create(context);
            p.rangeEdits["InMax"]->setRange(-1000000.F, 1000000.F);
            p.rangeEdits["InMax"]->setValue(range.max());

            p.sliders["Gamma"] = ftk::FloatEditSlider::create(context);
            p.sliders["Gamma"]->setRange(.1F, 4.F);
            p.sliders["Gamma"]->setDefaultValue(1.F);

            p.settings->getT("/Color/Levels/OutRange", range);
            p.sliders["OutLow"] = ftk::FloatEditSlider::create(context);
            p.sliders["OutLow"]->setRange(range);
            p.sliders["OutLow"]->setDefaultValue(0.F);

            p.sliders["OutHigh"] = ftk::FloatEditSlider::create(context);
            p.sliders["OutHigh"]->setRange(range);
            p.sliders["OutHigh"]->setDefaultValue(1.F);

            p.rangeEdits["OutMin"] = ftk::FloatEdit::create(context);
            p.rangeEdits["OutMin"]->setRange(-1000000.F, 1000000.F);
            p.rangeEdits["OutMin"]->setValue(range.min());

            p.rangeEdits["OutMax"] = ftk::FloatEdit::create(context);
            p.rangeEdits["OutMax"]->setRange(-1000000.F, 1000000.F);
            p.rangeEdits["OutMax"]->setValue(range.max());

            p.layout = ftk::FormLayout::create(context, shared_from_this());
            p.layout->setMarginRole(ftk::SizeRole::Margin);
            p.layout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.layout->addRow("Enabled:", p.enabledCheckBox);
            p.layout->addRow("In low:", p.sliders["InLow"]);
            p.layout->addRow("In high:", p.sliders["InHigh"]);
            auto hLayout = ftk::HorizontalLayout::create(context);
            hLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.rangeEdits["InMin"]->setParent(hLayout);
            p.rangeEdits["InMax"]->setParent(hLayout);
            p.layout->addRow("In range:", hLayout);
            p.layout->addRow("Gamma:", p.sliders["Gamma"]);
            p.layout->addRow("Out low:", p.sliders["OutLow"]);
            p.layout->addRow("Out high:", p.sliders["OutHigh"]);
            hLayout = ftk::HorizontalLayout::create(context);
            hLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.rangeEdits["OutMin"]->setParent(hLayout);
            p.rangeEdits["OutMax"]->setParent(hLayout);
            p.layout->addRow("Out range:", hLayout);

            p.optionsObservers = ftk::Observer<tl::timeline::DisplayOptions>::create(
                app->getViewportModel()->observeDisplayOptions(),
                [this](const tl::timeline::DisplayOptions& value)
                {
                    _p->enabledCheckBox->setChecked(value.levels.enabled);
                    _p->sliders["InLow"]->setValue(value.levels.inLow);
                    _p->sliders["InHigh"]->setValue(value.levels.inHigh);
                    _p->sliders["Gamma"]->setValue(value.levels.gamma);
                    _p->sliders["OutLow"]->setValue(value.levels.outLow);
                    _p->sliders["OutHigh"]->setValue(value.levels.outHigh);
                });

            auto appWeak = std::weak_ptr<App>(app);
            p.enabledCheckBox->setCheckedCallback(
                [appWeak](bool value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getViewportModel()->getDisplayOptions();
                        options.levels.enabled = value;
                        app->getViewportModel()->setDisplayOptions(options);
                    }
                });

            p.sliders["InLow"]->setCallback(
                [appWeak](float value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getViewportModel()->getDisplayOptions();
                        options.levels.enabled = true;
                        options.levels.inLow = value;
                        app->getViewportModel()->setDisplayOptions(options);
                    }
                });

            p.sliders["InHigh"]->setCallback(
                [appWeak](float value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getViewportModel()->getDisplayOptions();
                        options.levels.enabled = true;
                        options.levels.inHigh = value;
                        app->getViewportModel()->setDisplayOptions(options);
                    }
                });

            p.rangeEdits["InMin"]->setCallback(
                [this](float value)
                {
                    FTK_P();
                    ftk::RangeF range = p.sliders["InLow"]->getRange();
                    range = ftk::RangeF(value, range.max());
                    p.sliders["InLow"]->setRange(range);
                    p.sliders["InHigh"]->setRange(range);
                });

            p.rangeEdits["InMax"]->setCallback(
                [this](float value)
                {
                    FTK_P();
                    ftk::RangeF range = p.sliders["InLow"]->getRange();
                    range = ftk::RangeF(range.min(), value);
                    p.sliders["InLow"]->setRange(range);
                    p.sliders["InHigh"]->setRange(range);
                });

            p.sliders["Gamma"]->setCallback(
                [appWeak](float value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getViewportModel()->getDisplayOptions();
                        options.levels.enabled = true;
                        options.levels.gamma = value;
                        app->getViewportModel()->setDisplayOptions(options);
                    }
                });

            p.sliders["OutLow"]->setCallback(
                [appWeak](float value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getViewportModel()->getDisplayOptions();
                        options.levels.enabled = true;
                        options.levels.outLow = value;
                        app->getViewportModel()->setDisplayOptions(options);
                    }
                });

            p.sliders["OutHigh"]->setCallback(
                [appWeak](float value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getViewportModel()->getDisplayOptions();
                        options.levels.enabled = true;
                        options.levels.outHigh = value;
                        app->getViewportModel()->setDisplayOptions(options);
                    }
                });

            p.rangeEdits["OutMin"]->setCallback(
                [this](float value)
                {
                    FTK_P();
                    ftk::RangeF range = p.sliders["OutLow"]->getRange();
                    range = ftk::RangeF(value, range.max());
                    p.sliders["OutLow"]->setRange(range);
                    p.sliders["OutHigh"]->setRange(range);
                });

            p.rangeEdits["OutMax"]->setCallback(
                [this](float value)
                {
                    FTK_P();
                    ftk::RangeF range = p.sliders["OutLow"]->getRange();
                    range = ftk::RangeF(range.min(), value);
                    p.sliders["OutLow"]->setRange(range);
                    p.sliders["OutHigh"]->setRange(range);
                });
        }

        LevelsWidget::LevelsWidget() :
            _p(new Private)
        {}

        LevelsWidget::~LevelsWidget()
        {
            FTK_P();
            float min = p.rangeEdits["InMin"]->getValue();
            float max = p.rangeEdits["InMax"]->getValue();
            p.settings->setT("/Color/Levels/InRange", ftk::RangeF(min, max));
            min = p.rangeEdits["OutMin"]->getValue();
            max = p.rangeEdits["OutMax"]->getValue();
            p.settings->setT("/Color/Levels/OutRange", ftk::RangeF(min, max));
        }

        std::shared_ptr<LevelsWidget> LevelsWidget::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<LevelsWidget>(new LevelsWidget);
            out->_init(context, app, parent);
            return out;
        }

        ftk::Size2I LevelsWidget::getSizeHint() const
        {
            return _p->layout->getSizeHint();
        }

        void LevelsWidget::setGeometry(const ftk::Box2I& value)
        {
            IWidget::setGeometry(value);
            _p->layout->setGeometry(value);
        }

        struct EXRDisplayWidget::Private
        {
            std::shared_ptr<ftk::CheckBox> enabledCheckBox;
            std::map<std::string, std::shared_ptr<ftk::FloatEditSlider> > sliders;
            std::shared_ptr<ftk::FormLayout> layout;

            std::shared_ptr<ftk::Observer<tl::timeline::DisplayOptions> > optionsObservers;
        };

        void EXRDisplayWidget::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<ftk::IWidget>& parent)
        {
            ftk::IWidget::_init(context, "djv::app::EXRDisplayWidget", parent);
            FTK_P();

            p.enabledCheckBox = ftk::CheckBox::create(context);

            p.sliders["Exposure"] = ftk::FloatEditSlider::create(context);
            p.sliders["Exposure"]->setRange(-10.F, 10.F);
            p.sliders["Exposure"]->setDefaultValue(0.F);
            p.sliders["Defog"] = ftk::FloatEditSlider::create(context);
            p.sliders["Defog"]->setDefaultValue(0.F);
            p.sliders["KneeLow"] = ftk::FloatEditSlider::create(context);
            p.sliders["KneeLow"]->setRange(-3.F, 3.F);
            p.sliders["KneeLow"]->setDefaultValue(0.F);
            p.sliders["KneeHigh"] = ftk::FloatEditSlider::create(context);
            p.sliders["KneeHigh"]->setRange(3.5F, 7.5F);
            p.sliders["KneeHigh"]->setDefaultValue(5.F);

            p.layout = ftk::FormLayout::create(context, shared_from_this());
            p.layout->setMarginRole(ftk::SizeRole::Margin);
            p.layout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.layout->addRow("Enabled:", p.enabledCheckBox);
            p.layout->addRow("Exposure:", p.sliders["Exposure"]);
            p.layout->addRow("Defog:", p.sliders["Defog"]);
            p.layout->addRow("Knee low:", p.sliders["KneeLow"]);
            p.layout->addRow("Knee high:", p.sliders["KneeHigh"]);

            p.optionsObservers = ftk::Observer<tl::timeline::DisplayOptions>::create(
                app->getViewportModel()->observeDisplayOptions(),
                [this](const tl::timeline::DisplayOptions& value)
                {
                    _p->enabledCheckBox->setChecked(value.exrDisplay.enabled);
                    _p->sliders["Exposure"]->setValue(value.exrDisplay.exposure);
                    _p->sliders["Defog"]->setValue(value.exrDisplay.defog);
                    _p->sliders["KneeLow"]->setValue(value.exrDisplay.kneeLow);
                    _p->sliders["KneeHigh"]->setValue(value.exrDisplay.kneeHigh);
                });

            auto appWeak = std::weak_ptr<App>(app);
            p.enabledCheckBox->setCheckedCallback(
                [appWeak](bool value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getViewportModel()->getDisplayOptions();
                        options.exrDisplay.enabled = value;
                        app->getViewportModel()->setDisplayOptions(options);
                    }
                });

            p.sliders["Exposure"]->setCallback(
                [appWeak](float value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getViewportModel()->getDisplayOptions();
                        options.exrDisplay.enabled = true;
                        options.exrDisplay.exposure = value;
                        app->getViewportModel()->setDisplayOptions(options);
                    }
                });

            p.sliders["Defog"]->setCallback(
                [appWeak](float value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getViewportModel()->getDisplayOptions();
                        options.exrDisplay.enabled = true;
                        options.exrDisplay.defog = value;
                        app->getViewportModel()->setDisplayOptions(options);
                    }
                });

            p.sliders["KneeLow"]->setCallback(
                [appWeak](float value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getViewportModel()->getDisplayOptions();
                        options.exrDisplay.enabled = true;
                        options.exrDisplay.kneeLow = value;
                        app->getViewportModel()->setDisplayOptions(options);
                    }
                });

            p.sliders["KneeHigh"]->setCallback(
                [appWeak](float value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getViewportModel()->getDisplayOptions();
                        options.exrDisplay.enabled = true;
                        options.exrDisplay.kneeHigh = value;
                        app->getViewportModel()->setDisplayOptions(options);
                    }
                });
        }

        EXRDisplayWidget::EXRDisplayWidget() :
            _p(new Private)
        {}

        EXRDisplayWidget::~EXRDisplayWidget()
        {}

        std::shared_ptr<EXRDisplayWidget> EXRDisplayWidget::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<EXRDisplayWidget>(new EXRDisplayWidget);
            out->_init(context, app, parent);
            return out;
        }

        ftk::Size2I EXRDisplayWidget::getSizeHint() const
        {
            return _p->layout->getSizeHint();
        }

        void EXRDisplayWidget::setGeometry(const ftk::Box2I& value)
        {
            IWidget::setGeometry(value);
            _p->layout->setGeometry(value);
        }

        struct SoftClipWidget::Private
        {
            std::shared_ptr<ftk::CheckBox> enabledCheckBox;
            std::map<std::string, std::shared_ptr<ftk::FloatEditSlider> > sliders;
            std::shared_ptr<ftk::FormLayout> layout;

            std::shared_ptr<ftk::Observer<tl::timeline::DisplayOptions> > optionsObservers;
        };

        void SoftClipWidget::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<ftk::IWidget>& parent)
        {
            ftk::IWidget::_init(context, "djv::app::SoftClipWidget", parent);
            FTK_P();

            p.enabledCheckBox = ftk::CheckBox::create(context);

            p.sliders["SoftClip"] = ftk::FloatEditSlider::create(context);
            p.sliders["SoftClip"]->setDefaultValue(0.F);

            p.layout = ftk::FormLayout::create(context, shared_from_this());
            p.layout->setMarginRole(ftk::SizeRole::Margin);
            p.layout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.layout->addRow("Enabled:", p.enabledCheckBox);
            p.layout->addRow("Soft clip:", p.sliders["SoftClip"]);

            p.optionsObservers = ftk::Observer<tl::timeline::DisplayOptions>::create(
                app->getViewportModel()->observeDisplayOptions(),
                [this](const tl::timeline::DisplayOptions& value)
                {
                    _p->enabledCheckBox->setChecked(value.softClip.enabled);
                    _p->sliders["SoftClip"]->setValue(value.softClip.value);
                });

            auto appWeak = std::weak_ptr<App>(app);
            p.enabledCheckBox->setCheckedCallback(
                [appWeak](bool value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getViewportModel()->getDisplayOptions();
                        options.softClip.enabled = value;
                        app->getViewportModel()->setDisplayOptions(options);
                    }
                });

            p.sliders["SoftClip"]->setCallback(
                [appWeak](float value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getViewportModel()->getDisplayOptions();
                        options.softClip.enabled = true;
                        options.softClip.value = value;
                        app->getViewportModel()->setDisplayOptions(options);
                    }
                });
        }

        SoftClipWidget::SoftClipWidget() :
            _p(new Private)
        {}

        SoftClipWidget::~SoftClipWidget()
        {}

        std::shared_ptr<SoftClipWidget> SoftClipWidget::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<SoftClipWidget>(new SoftClipWidget);
            out->_init(context, app, parent);
            return out;
        }

        ftk::Size2I SoftClipWidget::getSizeHint() const
        {
            return _p->layout->getSizeHint();
        }

        void SoftClipWidget::setGeometry(const ftk::Box2I& value)
        {
            IWidget::setGeometry(value);
            _p->layout->setGeometry(value);
        }

        struct ColorTool::Private
        {
            std::shared_ptr<OCIOWidget> ocioWidget;
            std::shared_ptr<LUTWidget> lutWidget;
            std::shared_ptr<ColorWidget> colorWidget;
            std::shared_ptr<LevelsWidget> levelsWidget;
            std::shared_ptr<EXRDisplayWidget> exrDisplayWidget;
            std::shared_ptr<SoftClipWidget> softClipWidget;
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
                Tool::Color,
                "djv::app::ColorTool",
                parent);
            FTK_P();

            p.ocioWidget = OCIOWidget::create(context, app);
            p.lutWidget = LUTWidget::create(context, app);
            p.colorWidget = ColorWidget::create(context, app);
            p.levelsWidget = LevelsWidget::create(context, app);
            p.exrDisplayWidget = EXRDisplayWidget::create(context, app);
            p.softClipWidget = SoftClipWidget::create(context, app);

            auto layout = ftk::VerticalLayout::create(context);
            layout->setSpacingRole(ftk::SizeRole::None);
            p.bellows["OCIO"] = ftk::Bellows::create(context, "OCIO", layout);
            p.bellows["OCIO"]->setWidget(p.ocioWidget);
            p.bellows["LUT"] = ftk::Bellows::create(context, "LUT", layout);
            p.bellows["LUT"]->setWidget(p.lutWidget);
            p.bellows["Color"] = ftk::Bellows::create(context, "Color", layout);
            p.bellows["Color"]->setWidget(p.colorWidget);
            p.bellows["Levels"] = ftk::Bellows::create(context, "Levels", layout);
            p.bellows["Levels"]->setWidget(p.levelsWidget);
            p.bellows["EXRDisplay"] = ftk::Bellows::create(context, "EXR Display", layout);
            p.bellows["EXRDisplay"]->setWidget(p.exrDisplayWidget);
            p.bellows["SoftClip"] = ftk::Bellows::create(context, "Soft Clip", layout);
            p.bellows["SoftClip"]->setWidget(p.softClipWidget);
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
