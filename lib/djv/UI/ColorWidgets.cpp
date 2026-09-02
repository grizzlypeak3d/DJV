// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UI/ColorWidgets.h>

#include <djv/Models/ColorModel.h>
#include <djv/Models/ViewportModel.h>

#include <tlRender/Timeline/Util.h>

#include <ftk/UI/Bellows.h>
#include <ftk/UI/ButtonGroup.h>
#include <ftk/UI/CheckBox.h>
#include <ftk/UI/ComboBox.h>
#include <ftk/UI/FileEdit.h>
#include <ftk/UI/FloatEdit.h>
#include <ftk/UI/FloatEditSlider.h>
#include <ftk/UI/FormLayout.h>
#include <ftk/UI/IntEditSlider.h>
#include <ftk/UI/Label.h>
#include <ftk/UI/ToolButton.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/ScreenshotTag.h>
#include <ftk/UI/ScrollWidget.h>
#include <ftk/UI/Settings.h>
#include <ftk/UI/StackLayout.h>
#include <ftk/Core/Format.h>
#include <ftk/Core/String.h>

#include <cmath>

namespace djv
{
    namespace ui
    {
        struct OCIOWidget::Private
        {
            std::shared_ptr<models::ColorModel> colorModel;
            std::shared_ptr<models::OCIOModel> ocioModel;
            std::map<std::string, std::string> extColorSpaces;
            std::vector<std::string> colorSpaces;
            std::vector<std::string> exts;

            std::shared_ptr<ftk::CheckBox> enabledCheckBox;
            std::shared_ptr<ftk::ComboBox> configComboBox;
            std::shared_ptr<ftk::FileEdit> fileEdit;
            std::shared_ptr<ftk::Label> nameLabel;
            std::shared_ptr<ftk::ComboBox> inputComboBox;
            std::shared_ptr<ftk::Label> resolvedLabel;
            std::shared_ptr<ftk::ComboBox> displayComboBox;
            std::shared_ptr<ftk::ComboBox> viewComboBox;
            std::shared_ptr<ftk::ComboBox> lookComboBox;
            std::shared_ptr<ftk::ComboBox> extAddExtComboBox;
            std::shared_ptr<ftk::ComboBox> extAddColorSpaceComboBox;
            std::shared_ptr<ftk::ToolButton> extAddButton;
            std::shared_ptr<ftk::HorizontalLayout> extAddLayout;
            std::vector<std::shared_ptr<ftk::IWidget> > extRows;
            std::shared_ptr<ftk::FormLayout> formLayout;
            std::shared_ptr<ftk::VerticalLayout> layout;

            std::shared_ptr<ftk::Observer<tl::OCIOOptions> > optionsObserver;
            std::shared_ptr<ftk::Observer<tl::OCIOOptions> > optionsObserver2;
            std::shared_ptr<ftk::Observer<models::OCIOModelData> > dataObserver;
            std::shared_ptr<ftk::Observer<std::map<std::string, std::string> > > extObserver;
            std::shared_ptr<ftk::Observer<std::string> > resolvedObserver;
        };

        void OCIOWidget::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::ColorModel>& colorModel,
            const std::shared_ptr<ftk::IWidget>& parent)
        {
            ftk::IContainer::_init(context, "djv::ui::OCIOWidget", parent);
            FTK_P();

            p.colorModel = colorModel;
            p.ocioModel = models::OCIOModel::create(context);

            p.enabledCheckBox = ftk::CheckBox::create(context);
            p.enabledCheckBox->setTooltip("Toggle whether OCIO is enabled.");
            ftk::setScreenshotTag(p.enabledCheckBox, "Color.OCIO.Enabled");

            p.configComboBox = ftk::ComboBox::create(context, tl::getOCIOConfigLabels());
            p.configComboBox->setHStretch(ftk::Stretch::Expanding);
            ftk::setScreenshotTag(p.configComboBox, "Color.OCIO.Config");

            p.fileEdit = ftk::FileEdit::create(context);

            p.nameLabel = ftk::Label::create(context);
            ftk::setScreenshotTag(p.nameLabel, "Color.OCIO.Name");

            p.inputComboBox = ftk::ComboBox::create(context);
            p.inputComboBox->setHStretch(ftk::Stretch::Expanding);
            ftk::setScreenshotTag(p.inputComboBox, "Color.OCIO.Input");

            p.resolvedLabel = ftk::Label::create(context);
            p.resolvedLabel->setTooltip(
                "The input color space resolved for the current file, and "
                "where it came from. A file whose declared color space is "
                "not in the configuration says so here, and is shown "
                "unmanaged.");
            ftk::setScreenshotTag(p.resolvedLabel, "Color.OCIO.Resolved");

            p.displayComboBox = ftk::ComboBox::create(context);
            p.displayComboBox->setHStretch(ftk::Stretch::Expanding);
            ftk::setScreenshotTag(p.displayComboBox, "Color.OCIO.Display");

            p.viewComboBox = ftk::ComboBox::create(context);
            p.viewComboBox->setHStretch(ftk::Stretch::Expanding);
            ftk::setScreenshotTag(p.viewComboBox, "Color.OCIO.View");

            p.lookComboBox = ftk::ComboBox::create(context);
            p.lookComboBox->setHStretch(ftk::Stretch::Expanding);
            ftk::setScreenshotTag(p.lookComboBox, "Color.OCIO.Look");

            // Audio has no color, and timelines are containers of other
            // files, so an input color space for either would name nothing
            // that gets decoded.
            p.exts = tl::getExts(
                context,
                static_cast<int>(tl::FileType::Media) |
                static_cast<int>(tl::FileType::Seq));
            p.exts.erase(
                std::remove_if(
                    p.exts.begin(),
                    p.exts.end(),
                    [](const std::string& value)
                    {
                        return ".otio" == value || ".otioz" == value;
                    }),
                p.exts.end());
            p.extAddExtComboBox = ftk::ComboBox::create(context, p.exts);
            p.extAddExtComboBox->setHStretch(ftk::Stretch::Expanding);
            p.extAddExtComboBox->setTooltip(
                "File name extension to assign a color space to.");
            ftk::setScreenshotTag(p.extAddExtComboBox, "Color.OCIO.ExtAdd");

            p.extAddColorSpaceComboBox = ftk::ComboBox::create(context);
            p.extAddColorSpaceComboBox->setHStretch(ftk::Stretch::Expanding);
            p.extAddColorSpaceComboBox->setTooltip(
                "Color space to assign to the extension.");

            p.extAddButton = ftk::ToolButton::create(context, "Add");
            p.extAddButton->setTooltip(
                "Assign the color space to files with the extension.\n"
                "\n"
                "The assignments are used when the input color space is "
                "\"None\", and take precedence over the configuration's "
                "file rules.");

            p.layout = ftk::VerticalLayout::create(context);
            _setWidget(p.layout);
            p.layout->setMarginRole(ftk::SizeRole::Margin);
            p.layout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.formLayout = ftk::FormLayout::create(context, p.layout);
            p.formLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.formLayout->addRow("Configuration:", p.configComboBox);
            p.formLayout->addRow("File name:", p.fileEdit);
            p.formLayout->addRow("Name:", p.nameLabel);
            p.formLayout->addRow("Input:", p.inputComboBox);
            p.formLayout->addRow("Resolved:", p.resolvedLabel);
            p.formLayout->addRow("Display:", p.displayComboBox);
            p.formLayout->addRow("View:", p.viewComboBox);
            p.formLayout->addRow("Look:", p.lookComboBox);
            p.extAddLayout = ftk::HorizontalLayout::create(context);
            p.extAddLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.extAddExtComboBox->setParent(p.extAddLayout);
            p.extAddColorSpaceComboBox->setParent(p.extAddLayout);
            p.extAddButton->setParent(p.extAddLayout);
            p.formLayout->addRow("Extensions:", p.extAddLayout);

            p.optionsObserver = ftk::Observer<tl::OCIOOptions>::create(
                colorModel->observeOCIOOptions(),
                [this](const tl::OCIOOptions& value)
                {
                    _p->ocioModel->setOptions(value);
                });

            p.optionsObserver2 = ftk::Observer<tl::OCIOOptions>::create(
                p.ocioModel->observeOptions(),
                [colorModel](const tl::OCIOOptions& value)
                {
                    colorModel->setOCIOOptions(value);
                });

            p.dataObserver = ftk::Observer<models::OCIOModelData>::create(
                p.ocioModel->observeData(),
                [this](const models::OCIOModelData& value)
                {
                    FTK_P();
                    p.enabledCheckBox->setChecked(value.enabled);
                    p.configComboBox->setCurrentIndex(static_cast<int>(value.config));
                    p.fileEdit->setPath(ftk::Path(value.fileName));
                    p.nameLabel->setText(value.name);
                    p.formLayout->setRowVisible(p.fileEdit, tl::OCIOConfig::File == value.config);
                    p.inputComboBox->setItems(value.inputs);
                    p.inputComboBox->setCurrentIndex(value.inputIndex);
                    p.displayComboBox->setItems(value.displays);
                    p.displayComboBox->setCurrentIndex(value.displayIndex);
                    p.viewComboBox->setItems(value.views);
                    p.viewComboBox->setCurrentIndex(value.viewIndex);
                    p.lookComboBox->setItems(value.looks);
                    p.lookComboBox->setCurrentIndex(value.lookIndex);
                    // The first input is "Automatic", which is not a color
                    // space an extension can be assigned to.
                    std::vector<std::string> colorSpaces;
                    if (!value.inputs.empty())
                    {
                        colorSpaces.assign(value.inputs.begin() + 1, value.inputs.end());
                    }
                    if (colorSpaces != p.colorSpaces)
                    {
                        p.colorSpaces = colorSpaces;
                        p.extAddColorSpaceComboBox->setItems(p.colorSpaces);
                        _extUpdate();
                    }
                });

            p.enabledCheckBox->setCheckedCallback(
                [this](bool value)
                {
                    _p->ocioModel->setEnabled(value);
                });

            p.configComboBox->setIndexCallback(
                [this](int value)
                {
                    _p->ocioModel->setConfig(static_cast<tl::OCIOConfig>(value));
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

            p.extAddButton->setClickedCallback(
                [this]
                {
                    FTK_P();
                    const int extIndex = p.extAddExtComboBox->getCurrentIndex();
                    const int index = p.extAddColorSpaceComboBox->getCurrentIndex();
                    if (extIndex >= 0 &&
                        extIndex < static_cast<int>(p.exts.size()) &&
                        index >= 0 &&
                        index < static_cast<int>(p.colorSpaces.size()))
                    {
                        auto extColorSpaces = p.colorModel->getExtColorSpaces();
                        extColorSpaces[p.exts[extIndex]] = p.colorSpaces[index];
                        p.colorModel->setExtColorSpaces(extColorSpaces);
                    }
                });

            p.resolvedObserver = ftk::Observer<std::string>::create(
                colorModel->observeResolvedInput(),
                [this](const std::string& value)
                {
                    FTK_P();
                    p.resolvedLabel->setText(value);
                    p.formLayout->setRowVisible(p.resolvedLabel, !value.empty());
                });

            p.extObserver = ftk::Observer<std::map<std::string, std::string> >::create(
                colorModel->observeExtColorSpaces(),
                [this](const std::map<std::string, std::string>& value)
                {
                    _p->extColorSpaces = value;
                    _extUpdate();
                });
        }

        OCIOWidget::OCIOWidget() :
            _p(new Private)
        {}

        OCIOWidget::~OCIOWidget()
        {}

        std::shared_ptr<OCIOWidget> OCIOWidget::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::ColorModel>& colorModel,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<OCIOWidget>(new OCIOWidget);
            out->_init(context, colorModel, parent);
            return out;
        }

        std::shared_ptr<ftk::CheckBox> OCIOWidget::getEnabledCheckBox() const
        {
            return _p->enabledCheckBox;
        }

        void OCIOWidget::_extUpdate()
        {
            FTK_P();
            // The rows share the form layout with the rest of the widgets
            // so that everything lines up, which means rebuilding them is
            // removing the old rows and the add row, and appending the new
            // rows with the add row after them again.
            for (const auto& row : p.extRows)
            {
                p.formLayout->removeRow(row);
            }
            p.extRows.clear();
            p.formLayout->removeRow(p.extAddLayout);
            if (auto context = getContext())
            {
                for (const auto& i : p.extColorSpaces)
                {
                    const std::string ext = i.first;

                    auto hLayout = ftk::HorizontalLayout::create(context);
                    hLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);

                    auto comboBox = ftk::ComboBox::create(context, p.colorSpaces, hLayout);
                    comboBox->setHStretch(ftk::Stretch::Expanding);
                    const auto j = std::find(p.colorSpaces.begin(), p.colorSpaces.end(), i.second);
                    comboBox->setCurrentIndex(
                        j != p.colorSpaces.end() ? (j - p.colorSpaces.begin()) : -1);
                    comboBox->setIndexCallback(
                        [this, ext](int index)
                        {
                            FTK_P();
                            if (index >= 0 && index < static_cast<int>(p.colorSpaces.size()))
                            {
                                auto extColorSpaces = p.colorModel->getExtColorSpaces();
                                extColorSpaces[ext] = p.colorSpaces[index];
                                p.colorModel->setExtColorSpaces(extColorSpaces);
                            }
                        });

                    auto removeButton = ftk::ToolButton::create(context, hLayout);
                    removeButton->setIcon("CloseSmall");
                    removeButton->setTooltip("Remove the assignment.");
                    removeButton->setClickedCallback(
                        [this, ext]
                        {
                            FTK_P();
                            auto extColorSpaces = p.colorModel->getExtColorSpaces();
                            extColorSpaces.erase(ext);
                            p.colorModel->setExtColorSpaces(extColorSpaces);
                        });

                    p.formLayout->addRow(ext + ":", hLayout);
                    p.extRows.push_back(hLayout);
                }
            }
            p.formLayout->addRow("Extensions:", p.extAddLayout);
        }

        struct LUTWidget::Private
        {
            std::shared_ptr<ftk::CheckBox> enabledCheckBox;
            std::shared_ptr<ftk::FileEdit> fileEdit;
            std::shared_ptr<ftk::ComboBox> orderComboBox;
            std::shared_ptr<ftk::FormLayout> layout;

            std::shared_ptr<ftk::Observer<tl::LUTOptions> > optionsObservers;
        };

        void LUTWidget::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::ColorModel>& colorModel,
            const std::shared_ptr<ftk::IWidget>& parent)
        {
            ftk::IContainer::_init(context, "djv::ui::LUTWidget", parent);
            FTK_P();

            p.enabledCheckBox = ftk::CheckBox::create(context);
            p.enabledCheckBox->setTooltip("Toggle whether the LUT is enabled.");
            ftk::setScreenshotTag(p.enabledCheckBox, "Color.LUT.Enabled");

            p.fileEdit = ftk::FileEdit::create(context);
            std::vector<std::string> s;
            const auto lutFormatNames = tl::getLUTFormatNames();
            const auto lutFormatExts = tl::getLUTFormatExts();
            for (size_t i = 0; i < lutFormatNames.size() && i < lutFormatExts.size(); ++i)
            {
                s.push_back(ftk::Format("* {0}: {1}").
                    arg(lutFormatNames[i]).
                    arg(lutFormatExts[i]));
            }
            p.fileEdit->setTooltip(ftk::Format("Supported LUT formats:\n{0}").arg(ftk::join(s, '\n')));
            ftk::setScreenshotTag(p.fileEdit, "Color.LUT.File");

            p.orderComboBox = ftk::ComboBox::create(context, tl::getLUTOrderLabels());
            p.orderComboBox->setHStretch(ftk::Stretch::Expanding);
            ftk::setScreenshotTag(p.orderComboBox, "Color.LUT.Order");

            p.layout = ftk::FormLayout::create(context);
            _setWidget(p.layout);
            p.layout->setMarginRole(ftk::SizeRole::Margin);
            p.layout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.layout->addRow("File name:", p.fileEdit);
            p.layout->addRow("Order:", p.orderComboBox);

            p.optionsObservers = ftk::Observer<tl::LUTOptions>::create(
                colorModel->observeLUTOptions(),
                [this](const tl::LUTOptions& value)
                {
                    _p->enabledCheckBox->setChecked(value.enabled);
                    _p->fileEdit->setPath(ftk::Path(value.fileName));
                    _p->orderComboBox->setCurrentIndex(static_cast<size_t>(value.order));
                });

            p.enabledCheckBox->setCheckedCallback(
                [colorModel](bool value)
                {
                    auto options = colorModel->getLUTOptions();
                    options.enabled = value;
                    colorModel->setLUTOptions(options);
                });

            p.fileEdit->setCallback(
                [colorModel](const ftk::Path& value)
                {
                    auto options = colorModel->getLUTOptions();
                    options.enabled = true;
                    options.fileName = value.get();
                    colorModel->setLUTOptions(options);
                });

            p.orderComboBox->setIndexCallback(
                [colorModel](int value)
                {
                    auto options = colorModel->getLUTOptions();
                    options.enabled = true;
                    options.order = static_cast<tl::LUTOrder>(value);
                    colorModel->setLUTOptions(options);
                });
        }

        LUTWidget::LUTWidget() :
            _p(new Private)
        {}

        LUTWidget::~LUTWidget()
        {}

        std::shared_ptr<LUTWidget> LUTWidget::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::ColorModel>& colorModel,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<LUTWidget>(new LUTWidget);
            out->_init(context, colorModel, parent);
            return out;
        }

        std::shared_ptr<ftk::CheckBox> LUTWidget::getEnabledCheckBox() const
        {
            return _p->enabledCheckBox;
        }

        struct ColorWidget::Private
        {
            std::shared_ptr<ftk::CheckBox> enabledCheckBox;
            std::map<std::string, std::shared_ptr<ftk::FloatEditSlider> > sliders;
            std::shared_ptr<ftk::IntEditSlider> hueSlider;
            std::shared_ptr<ftk::FormLayout> layout;

            std::shared_ptr<ftk::Observer<tl::DisplayOptions> > optionsObservers;
        };

        void ColorWidget::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::ViewportModel>& viewportModel,
            const std::shared_ptr<ftk::IWidget>& parent)
        {
            ftk::IContainer::_init(context, "djv::ui::ColorWidget", parent);
            FTK_P();

            p.enabledCheckBox = ftk::CheckBox::create(context);
            p.enabledCheckBox->setTooltip("Toggle whether color controls are enabled.");

            p.sliders["Add"] = ftk::FloatEditSlider::create(context);
            p.sliders["Add"]->setRange(-1.F, 1.F);
            p.sliders["Add"]->setDefault(0.F);
            p.sliders["Add"]->getModel()->setRangeSoft(true);
            ftk::setScreenshotTag(p.sliders["Add"], "Color.Controls.Add");

            p.sliders["Brightness"] = ftk::FloatEditSlider::create(context);
            p.sliders["Brightness"]->setRange(0.F, 4.F);
            p.sliders["Brightness"]->setDefault(1.F);
            p.sliders["Brightness"]->getModel()->setRangeSoft(true);
            ftk::setScreenshotTag(p.sliders["Brightness"], "Color.Controls.Brightness");

            p.sliders["Contrast"] = ftk::FloatEditSlider::create(context);
            p.sliders["Contrast"]->setRange(0.F, 4.F);
            p.sliders["Contrast"]->setDefault(1.F);
            p.sliders["Contrast"]->getModel()->setRangeSoft(true);
            ftk::setScreenshotTag(p.sliders["Contrast"], "Color.Controls.Contrast");

            p.sliders["Saturation"] = ftk::FloatEditSlider::create(context);
            p.sliders["Saturation"]->setRange(0.F, 4.F);
            p.sliders["Saturation"]->setDefault(1.F);
            p.sliders["Saturation"]->getModel()->setRangeSoft(true);
            ftk::setScreenshotTag(p.sliders["Saturation"], "Color.Controls.Saturation");

            p.hueSlider = ftk::IntEditSlider::create(context);
            p.hueSlider->setRange(0, 360);
            p.hueSlider->setStep(10);
            p.hueSlider->setLargeStep(60);
            p.hueSlider->setDefault(0);
            ftk::setScreenshotTag(p.hueSlider, "Color.Controls.Hue");

            p.layout = ftk::FormLayout::create(context);
            _setWidget(p.layout);
            p.layout->setMarginRole(ftk::SizeRole::Margin);
            p.layout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.layout->addRow("Add:", p.sliders["Add"]);
            p.layout->addRow("Brightness:", p.sliders["Brightness"]);
            p.layout->addRow("Contrast:", p.sliders["Contrast"]);
            p.layout->addRow("Saturation:", p.sliders["Saturation"]);
            p.layout->addRow("Hue:", p.hueSlider);

            p.optionsObservers = ftk::Observer<tl::DisplayOptions>::create(
                viewportModel->observeDisplayOptions(),
                [this](const tl::DisplayOptions& value)
                {
                    FTK_P();
                    p.enabledCheckBox->setChecked(value.color.enabled);
                    p.sliders["Add"]->setValue(value.color.add.x);
                    p.sliders["Brightness"]->setValue(value.color.brightness.x);
                    p.sliders["Contrast"]->setValue(value.color.contrast.x);
                    p.sliders["Saturation"]->setValue(value.color.saturation.x);
                    p.hueSlider->setValue(std::round(value.color.hue * 360.F));
                });

            p.enabledCheckBox->setCheckedCallback(
                [viewportModel](bool value)
                {
                    auto options = viewportModel->getDisplayOptions();
                    options.color.enabled = value;
                    viewportModel->setDisplayOptions(options);
                });

            p.sliders["Add"]->setCallback(
                [viewportModel](float value)
                {
                    auto options = viewportModel->getDisplayOptions();
                    // A value echoed back by the options observer
                    // changes nothing; without this check the echo
                    // would enable the stage.
                    if (value == options.color.add.x)
                    {
                        return;
                    }
                    options.color.enabled = true;
                    options.color.add.x = value;
                    options.color.add.y = value;
                    options.color.add.z = value;
                    viewportModel->setDisplayOptions(options);
                });

            p.sliders["Brightness"]->setCallback(
                [viewportModel](float value)
                {
                    auto options = viewportModel->getDisplayOptions();
                    if (value == options.color.brightness.x)
                    {
                        return;
                    }
                    options.color.enabled = true;
                    options.color.brightness.x = value;
                    options.color.brightness.y = value;
                    options.color.brightness.z = value;
                    viewportModel->setDisplayOptions(options);
                });

            p.sliders["Contrast"]->setCallback(
                [viewportModel](float value)
                {
                    auto options = viewportModel->getDisplayOptions();
                    if (value == options.color.contrast.x)
                    {
                        return;
                    }
                    options.color.enabled = true;
                    options.color.contrast.x = value;
                    options.color.contrast.y = value;
                    options.color.contrast.z = value;
                    viewportModel->setDisplayOptions(options);
                });

            p.sliders["Saturation"]->setCallback(
                [viewportModel](float value)
                {
                    auto options = viewportModel->getDisplayOptions();
                    if (value == options.color.saturation.x)
                    {
                        return;
                    }
                    options.color.enabled = true;
                    options.color.saturation.x = value;
                    options.color.saturation.y = value;
                    options.color.saturation.z = value;
                    viewportModel->setDisplayOptions(options);
                });

            p.hueSlider->setCallback(
                [viewportModel](int value)
                {
                    auto options = viewportModel->getDisplayOptions();
                    if (value == static_cast<int>(std::round(options.color.hue * 360.F)))
                    {
                        return;
                    }
                    options.color.enabled = true;
                    options.color.hue = value / 360.F;
                    viewportModel->setDisplayOptions(options);
                });
        }

        ColorWidget::ColorWidget() :
            _p(new Private)
        {}

        ColorWidget::~ColorWidget()
        {}

        std::shared_ptr<ColorWidget> ColorWidget::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::ViewportModel>& viewportModel,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<ColorWidget>(new ColorWidget);
            out->_init(context, viewportModel, parent);
            return out;
        }

        std::shared_ptr<ftk::CheckBox> ColorWidget::getEnabledCheckBox() const
        {
            return _p->enabledCheckBox;
        }

        struct LevelsWidget::Private
        {
            std::shared_ptr<ftk::Settings> settings;

            std::shared_ptr<ftk::CheckBox> enabledCheckBox;
            std::map<std::string, std::shared_ptr<ftk::FloatEditSlider> > sliders;
            std::shared_ptr<ftk::FormLayout> layout;

            std::shared_ptr<ftk::Observer<tl::DisplayOptions> > optionsObservers;
            std::map<std::string, std::shared_ptr<ftk::Observer<ftk::RangeF> > > rangeObservers;
        };

        void LevelsWidget::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<ftk::Settings>& settings,
            const std::shared_ptr<models::ViewportModel>& viewportModel,
            const std::shared_ptr<ftk::IWidget>& parent)
        {
            ftk::IContainer::_init(context, "djv::ui::LevelsWidget", parent);
            FTK_P();

            p.settings = settings;

            p.enabledCheckBox = ftk::CheckBox::create(context);
            p.enabledCheckBox->setTooltip("Toggle whether levels are enabled.");
            ftk::setScreenshotTag(p.enabledCheckBox, "Color.Levels.Enabled");

            // The in and out sliders get soft ranges so that values
            // typed or set beyond the range extend it, the way the
            // dedicated range edits used to.
            ftk::RangeF range(0.F, 1.F);
            p.settings->getT("/Color/Levels/InRange", range);
            p.sliders["InLow"] = ftk::FloatEditSlider::create(context);
            p.sliders["InLow"]->setRange(range);
            p.sliders["InLow"]->setDefault(0.F);
            p.sliders["InLow"]->getModel()->setRangeSoft(true);
            ftk::setScreenshotTag(p.sliders["InLow"], "Color.Levels.In");

            p.sliders["InHigh"] = ftk::FloatEditSlider::create(context);
            p.sliders["InHigh"]->setRange(range);
            p.sliders["InHigh"]->setDefault(1.F);
            p.sliders["InHigh"]->getModel()->setRangeSoft(true);
            ftk::setScreenshotTag(p.sliders["InHigh"], "Color.Levels.In");

            p.sliders["Gamma"] = ftk::FloatEditSlider::create(context);
            p.sliders["Gamma"]->setRange(.1F, 4.F);
            p.sliders["Gamma"]->setDefault(1.F);
            p.sliders["Gamma"]->getModel()->setRangeSoft(true);
            ftk::setScreenshotTag(p.sliders["Gamma"], "Color.Levels.Gamma");

            p.settings->getT("/Color/Levels/OutRange", range);
            p.sliders["OutLow"] = ftk::FloatEditSlider::create(context);
            p.sliders["OutLow"]->setRange(range);
            p.sliders["OutLow"]->setDefault(0.F);
            p.sliders["OutLow"]->getModel()->setRangeSoft(true);
            ftk::setScreenshotTag(p.sliders["OutLow"], "Color.Levels.Out");

            p.sliders["OutHigh"] = ftk::FloatEditSlider::create(context);
            p.sliders["OutHigh"]->setRange(range);
            p.sliders["OutHigh"]->setDefault(1.F);
            p.sliders["OutHigh"]->getModel()->setRangeSoft(true);
            ftk::setScreenshotTag(p.sliders["OutHigh"], "Color.Levels.Out");

            p.layout = ftk::FormLayout::create(context);
            _setWidget(p.layout);
            p.layout->setMarginRole(ftk::SizeRole::Margin);
            p.layout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.layout->addRow("In low:", p.sliders["InLow"]);
            p.layout->addRow("In high:", p.sliders["InHigh"]);
            p.layout->addRow("Gamma:", p.sliders["Gamma"]);
            p.layout->addRow("Out low:", p.sliders["OutLow"]);
            p.layout->addRow("Out high:", p.sliders["OutHigh"]);

            p.optionsObservers = ftk::Observer<tl::DisplayOptions>::create(
                viewportModel->observeDisplayOptions(),
                [this](const tl::DisplayOptions& value)
                {
                    _p->enabledCheckBox->setChecked(value.levels.enabled);
                    _p->sliders["InLow"]->setValue(value.levels.inLow);
                    _p->sliders["InHigh"]->setValue(value.levels.inHigh);
                    _p->sliders["Gamma"]->setValue(value.levels.gamma);
                    _p->sliders["OutLow"]->setValue(value.levels.outLow);
                    _p->sliders["OutHigh"]->setValue(value.levels.outHigh);
                });

            p.enabledCheckBox->setCheckedCallback(
                [viewportModel](bool value)
                {
                    auto options = viewportModel->getDisplayOptions();
                    options.levels.enabled = value;
                    viewportModel->setDisplayOptions(options);
                });

            p.sliders["InLow"]->setCallback(
                [viewportModel](float value)
                {
                    auto options = viewportModel->getDisplayOptions();
                    if (value == options.levels.inLow)
                    {
                        return;
                    }
                    options.levels.enabled = true;
                    options.levels.inLow = value;
                    viewportModel->setDisplayOptions(options);
                });

            p.sliders["InHigh"]->setCallback(
                [viewportModel](float value)
                {
                    auto options = viewportModel->getDisplayOptions();
                    if (value == options.levels.inHigh)
                    {
                        return;
                    }
                    options.levels.enabled = true;
                    options.levels.inHigh = value;
                    viewportModel->setDisplayOptions(options);
                });

            // Keep the low and high sliders of a pair on the same range,
            // the way the shared range edits used to; extending one
            // extends the other. The observers do not loop because
            // setting an unchanged range does not notify. The range goes
            // through the model, not the composite: the composite blocks
            // its callbacks around programmatic changes, so when a
            // shrinking range clamps the sibling's value, the new value
            // would never reach the display options.
            p.rangeObservers["InLow"] = ftk::Observer<ftk::RangeF>::create(
                p.sliders["InLow"]->getModel()->observeRange(),
                [this](const ftk::RangeF& value)
                {
                    _p->sliders["InHigh"]->getModel()->setRange(value);
                });

            p.rangeObservers["InHigh"] = ftk::Observer<ftk::RangeF>::create(
                p.sliders["InHigh"]->getModel()->observeRange(),
                [this](const ftk::RangeF& value)
                {
                    _p->sliders["InLow"]->getModel()->setRange(value);
                });

            p.sliders["Gamma"]->setCallback(
                [viewportModel](float value)
                {
                    auto options = viewportModel->getDisplayOptions();
                    if (value == options.levels.gamma)
                    {
                        return;
                    }
                    options.levels.enabled = true;
                    options.levels.gamma = value;
                    viewportModel->setDisplayOptions(options);
                });

            p.sliders["OutLow"]->setCallback(
                [viewportModel](float value)
                {
                    auto options = viewportModel->getDisplayOptions();
                    if (value == options.levels.outLow)
                    {
                        return;
                    }
                    options.levels.enabled = true;
                    options.levels.outLow = value;
                    viewportModel->setDisplayOptions(options);
                });

            p.sliders["OutHigh"]->setCallback(
                [viewportModel](float value)
                {
                    auto options = viewportModel->getDisplayOptions();
                    if (value == options.levels.outHigh)
                    {
                        return;
                    }
                    options.levels.enabled = true;
                    options.levels.outHigh = value;
                    viewportModel->setDisplayOptions(options);
                });

            p.rangeObservers["OutLow"] = ftk::Observer<ftk::RangeF>::create(
                p.sliders["OutLow"]->getModel()->observeRange(),
                [this](const ftk::RangeF& value)
                {
                    _p->sliders["OutHigh"]->getModel()->setRange(value);
                });

            p.rangeObservers["OutHigh"] = ftk::Observer<ftk::RangeF>::create(
                p.sliders["OutHigh"]->getModel()->observeRange(),
                [this](const ftk::RangeF& value)
                {
                    _p->sliders["OutLow"]->getModel()->setRange(value);
                });
        }

        LevelsWidget::LevelsWidget() :
            _p(new Private)
        {}

        LevelsWidget::~LevelsWidget()
        {
            FTK_P();
            p.settings->setT("/Color/Levels/InRange", p.sliders["InLow"]->getRange());
            p.settings->setT("/Color/Levels/OutRange", p.sliders["OutLow"]->getRange());
        }

        std::shared_ptr<LevelsWidget> LevelsWidget::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<ftk::Settings>& settings,
            const std::shared_ptr<models::ViewportModel>& viewportModel,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<LevelsWidget>(new LevelsWidget);
            out->_init(context, settings, viewportModel, parent);
            return out;
        }

        std::shared_ptr<ftk::CheckBox> LevelsWidget::getEnabledCheckBox() const
        {
            return _p->enabledCheckBox;
        }

        struct ExposureWidget::Private
        {
            std::shared_ptr<ftk::CheckBox> enabledCheckBox;
            std::map<std::string, std::shared_ptr<ftk::FloatEditSlider> > sliders;
            std::shared_ptr<ftk::FormLayout> layout;

            std::shared_ptr<ftk::Observer<tl::DisplayOptions> > optionsObservers;
        };

        void ExposureWidget::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::ViewportModel>& viewportModel,
            const std::shared_ptr<ftk::IWidget>& parent)
        {
            ftk::IContainer::_init(context, "djv::ui::ExposureWidget", parent);
            FTK_P();

            p.enabledCheckBox = ftk::CheckBox::create(context);
            p.enabledCheckBox->setTooltip("Toggle whether exposure controls are enabled.");
            ftk::setScreenshotTag(p.enabledCheckBox, "Color.Exposure.Enabled");

            p.sliders["Exposure"] = ftk::FloatEditSlider::create(context);
            p.sliders["Exposure"]->setRange(-10.F, 10.F);
            p.sliders["Exposure"]->setDefault(0.F);
            p.sliders["Exposure"]->getModel()->setRangeSoft(true);
            ftk::setScreenshotTag(p.sliders["Exposure"], "Color.Exposure.Exposure");

            p.sliders["Defog"] = ftk::FloatEditSlider::create(context);
            p.sliders["Defog"]->setDefault(0.F);
            p.sliders["Defog"]->getModel()->setRangeSoft(true);
            ftk::setScreenshotTag(p.sliders["Defog"], "Color.Exposure.Defog");

            p.sliders["KneeLow"] = ftk::FloatEditSlider::create(context);
            p.sliders["KneeLow"]->setRange(-3.F, 3.F);
            p.sliders["KneeLow"]->setDefault(0.F);
            p.sliders["KneeLow"]->getModel()->setRangeSoft(true);
            ftk::setScreenshotTag(p.sliders["KneeLow"], "Color.Exposure.KneeLow");

            p.sliders["KneeHigh"] = ftk::FloatEditSlider::create(context);
            p.sliders["KneeHigh"]->setRange(3.5F, 7.5F);
            p.sliders["KneeHigh"]->setDefault(5.F);
            p.sliders["KneeHigh"]->getModel()->setRangeSoft(true);
            ftk::setScreenshotTag(p.sliders["KneeHigh"], "Color.Exposure.KneeHigh");

            p.sliders["Gamma"] = ftk::FloatEditSlider::create(context);
            p.sliders["Gamma"]->setRange(.1F, 4.F);
            p.sliders["Gamma"]->setDefault(1.F);
            p.sliders["Gamma"]->getModel()->setRangeSoft(true);
            ftk::setScreenshotTag(p.sliders["Gamma"], "Color.Exposure.Gamma");

            p.layout = ftk::FormLayout::create(context);
            _setWidget(p.layout);
            p.layout->setMarginRole(ftk::SizeRole::Margin);
            p.layout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.layout->addRow("Exposure:", p.sliders["Exposure"]);
            p.layout->addRow("Defog:", p.sliders["Defog"]);
            p.layout->addRow("Knee low:", p.sliders["KneeLow"]);
            p.layout->addRow("Knee high:", p.sliders["KneeHigh"]);
            p.layout->addRow("Gamma:", p.sliders["Gamma"]);

            p.optionsObservers = ftk::Observer<tl::DisplayOptions>::create(
                viewportModel->observeDisplayOptions(),
                [this](const tl::DisplayOptions& value)
                {
                    _p->enabledCheckBox->setChecked(value.exposure.enabled);
                    _p->sliders["Exposure"]->setValue(value.exposure.exposure);
                    _p->sliders["Defog"]->setValue(value.exposure.defog);
                    _p->sliders["KneeLow"]->setValue(value.exposure.kneeLow);
                    _p->sliders["KneeHigh"]->setValue(value.exposure.kneeHigh);
                    _p->sliders["Gamma"]->setValue(value.exposure.gamma);
                });

            p.enabledCheckBox->setCheckedCallback(
                [viewportModel](bool value)
                {
                    auto options = viewportModel->getDisplayOptions();
                    options.exposure.enabled = value;
                    viewportModel->setDisplayOptions(options);
                });

            p.sliders["Exposure"]->setCallback(
                [viewportModel](float value)
                {
                    auto options = viewportModel->getDisplayOptions();
                    if (value == options.exposure.exposure)
                    {
                        return;
                    }
                    options.exposure.enabled = true;
                    options.exposure.exposure = value;
                    viewportModel->setDisplayOptions(options);
                });

            p.sliders["Defog"]->setCallback(
                [viewportModel](float value)
                {
                    auto options = viewportModel->getDisplayOptions();
                    if (value == options.exposure.defog)
                    {
                        return;
                    }
                    options.exposure.enabled = true;
                    options.exposure.defog = value;
                    viewportModel->setDisplayOptions(options);
                });

            p.sliders["KneeLow"]->setCallback(
                [viewportModel](float value)
                {
                    auto options = viewportModel->getDisplayOptions();
                    if (value == options.exposure.kneeLow)
                    {
                        return;
                    }
                    options.exposure.enabled = true;
                    options.exposure.kneeLow = value;
                    viewportModel->setDisplayOptions(options);
                });

            p.sliders["KneeHigh"]->setCallback(
                [viewportModel](float value)
                {
                    auto options = viewportModel->getDisplayOptions();
                    if (value == options.exposure.kneeHigh)
                    {
                        return;
                    }
                    options.exposure.enabled = true;
                    options.exposure.kneeHigh = value;
                    viewportModel->setDisplayOptions(options);
                });

            p.sliders["Gamma"]->setCallback(
                [viewportModel](float value)
                {
                    auto options = viewportModel->getDisplayOptions();
                    if (value == options.exposure.gamma)
                    {
                        return;
                    }
                    options.exposure.enabled = true;
                    options.exposure.gamma = value;
                    viewportModel->setDisplayOptions(options);
                });
        }

        ExposureWidget::ExposureWidget() :
            _p(new Private)
        {}

        ExposureWidget::~ExposureWidget()
        {}

        std::shared_ptr<ExposureWidget> ExposureWidget::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::ViewportModel>& viewportModel,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<ExposureWidget>(new ExposureWidget);
            out->_init(context, viewportModel, parent);
            return out;
        }

        std::shared_ptr<ftk::CheckBox> ExposureWidget::getEnabledCheckBox() const
        {
            return _p->enabledCheckBox;
        }

        struct SoftClipWidget::Private
        {
            std::shared_ptr<ftk::CheckBox> enabledCheckBox;
            std::map<std::string, std::shared_ptr<ftk::FloatEditSlider> > sliders;
            std::shared_ptr<ftk::FormLayout> layout;

            std::shared_ptr<ftk::Observer<tl::DisplayOptions> > optionsObservers;
        };

        void SoftClipWidget::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::ViewportModel>& viewportModel,
            const std::shared_ptr<ftk::IWidget>& parent)
        {
            ftk::IContainer::_init(context, "djv::ui::SoftClipWidget", parent);
            FTK_P();

            p.enabledCheckBox = ftk::CheckBox::create(context);
            p.enabledCheckBox->setTooltip("Toggle whether soft clip is enabled.");
            ftk::setScreenshotTag(p.enabledCheckBox, "Color.SoftClip.Enabled");

            p.sliders["SoftClip"] = ftk::FloatEditSlider::create(context);
            p.sliders["SoftClip"]->setDefault(0.F);
            ftk::setScreenshotTag(p.sliders["SoftClip"], "Color.SoftClip.Value");

            p.layout = ftk::FormLayout::create(context);
            _setWidget(p.layout);
            p.layout->setMarginRole(ftk::SizeRole::Margin);
            p.layout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.layout->addRow("Soft clip:", p.sliders["SoftClip"]);

            p.optionsObservers = ftk::Observer<tl::DisplayOptions>::create(
                viewportModel->observeDisplayOptions(),
                [this](const tl::DisplayOptions& value)
                {
                    _p->enabledCheckBox->setChecked(value.softClip.enabled);
                    _p->sliders["SoftClip"]->setValue(value.softClip.value);
                });

            p.enabledCheckBox->setCheckedCallback(
                [viewportModel](bool value)
                {
                    auto options = viewportModel->getDisplayOptions();
                    options.softClip.enabled = value;
                    viewportModel->setDisplayOptions(options);
                });

            p.sliders["SoftClip"]->setCallback(
                [viewportModel](float value)
                {
                    auto options = viewportModel->getDisplayOptions();
                    if (value == options.softClip.value)
                    {
                        return;
                    }
                    options.softClip.enabled = true;
                    options.softClip.value = value;
                    viewportModel->setDisplayOptions(options);
                });
        }

        SoftClipWidget::SoftClipWidget() :
            _p(new Private)
        {}

        SoftClipWidget::~SoftClipWidget()
        {}

        std::shared_ptr<SoftClipWidget> SoftClipWidget::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::ViewportModel>& viewportModel,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<SoftClipWidget>(new SoftClipWidget);
            out->_init(context, viewportModel, parent);
            return out;
        }

        std::shared_ptr<ftk::CheckBox> SoftClipWidget::getEnabledCheckBox() const
        {
            return _p->enabledCheckBox;
        }
    }
}
