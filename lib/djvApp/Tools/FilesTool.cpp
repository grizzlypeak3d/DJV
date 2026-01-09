// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djvApp/Tools/FilesToolPrivate.h>

#include <djvApp/App.h>

#include <ftk/UI/Bellows.h>
#include <ftk/UI/ButtonGroup.h>
#include <ftk/UI/ComboBox.h>
#include <ftk/UI/Divider.h>
#include <ftk/UI/FloatEditSlider.h>
#include <ftk/UI/FormLayout.h>
#include <ftk/UI/GridLayout.h>
#include <ftk/UI/Label.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/ScrollWidget.h>
#include <ftk/UI/Settings.h>
#include <ftk/UI/ToolButton.h>

namespace djv
{
    namespace app
    {
        struct FilesTool::Private
        {
            std::shared_ptr<ftk::Settings> settings;

            std::shared_ptr<ftk::ButtonGroup> aButtonGroup;
            std::shared_ptr<ftk::ButtonGroup> bButtonGroup;
            std::map<std::shared_ptr<FilesModelItem>, std::shared_ptr<FileButton> > aButtons;
            std::map<std::shared_ptr<FilesModelItem>, std::shared_ptr<ftk::ToolButton> > bButtons;
            std::vector<std::shared_ptr<ftk::ComboBox> > layerComboBoxes;
            std::shared_ptr<ftk::ComboBox> compareComboBox;
            std::shared_ptr<ftk::ComboBox> compareTimeComboBox;
            std::shared_ptr<ftk::FloatEditSlider> wipeXSlider;
            std::shared_ptr<ftk::FloatEditSlider> wipeYSlider;
            std::shared_ptr<ftk::FloatEditSlider> wipeRotationSlider;
            std::shared_ptr<ftk::FloatEditSlider> overlaySlider;
            std::shared_ptr<ftk::FormLayout> compareLayout;
            std::map<std::string, std::shared_ptr<ftk::Bellows> > bellows;
            std::shared_ptr<ftk::GridLayout> widgetLayout;

            std::shared_ptr<ftk::ListObserver<std::shared_ptr<FilesModelItem> > > filesObserver;
            std::shared_ptr<ftk::Observer<std::shared_ptr<FilesModelItem> > > aObserver;
            std::shared_ptr<ftk::ListObserver<std::shared_ptr<FilesModelItem> > > bObserver;
            std::shared_ptr<ftk::ListObserver<int> > layersObserver;
            std::shared_ptr<ftk::Observer<tl::CompareOptions> > compareObserver;
            std::shared_ptr<ftk::Observer<tl::CompareTime> > compareTimeObserver;
        };

        void FilesTool::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            IToolWidget::_init(
                context,
                app,
                Tool::Files,
                "djv::app::FilesTool",
                parent);
            FTK_P();

            p.settings = app->getSettings();

            p.aButtonGroup = ftk::ButtonGroup::create(context, ftk::ButtonGroupType::Radio);
            p.bButtonGroup = ftk::ButtonGroup::create(context, ftk::ButtonGroupType::Check);

            p.compareComboBox = ftk::ComboBox::create(
                context,
                tl::getCompareLabels());
            p.compareComboBox->setHStretch(ftk::Stretch::Expanding);
            p.compareTimeComboBox = ftk::ComboBox::create(
                context,
                tl::getCompareTimeLabels());
            p.compareTimeComboBox->setHStretch(ftk::Stretch::Expanding);

            p.wipeXSlider = ftk::FloatEditSlider::create(context);
            p.wipeXSlider->setDefaultValue(.5F);
            p.wipeYSlider = ftk::FloatEditSlider::create(context);
            p.wipeYSlider->setDefaultValue(.5F);
            p.wipeRotationSlider = ftk::FloatEditSlider::create(context);
            p.wipeRotationSlider->setRange(0.F, 360.F);
            p.wipeRotationSlider->setStep(1.F);
            p.wipeRotationSlider->setLargeStep(10.F);
            p.wipeRotationSlider->setDefaultValue(0.F);

            p.overlaySlider = ftk::FloatEditSlider::create(context);
            p.overlaySlider->setDefaultValue(.5F);

            auto layout = ftk::VerticalLayout::create(context);
            layout->setSpacingRole(ftk::SizeRole::None);

            p.widgetLayout = ftk::GridLayout::create(context, layout);
            p.widgetLayout->setMarginRole(ftk::SizeRole::Margin);
            p.widgetLayout->setSpacingRole(ftk::SizeRole::SpacingTool);

            ftk::Divider::create(context, ftk::Orientation::Vertical, layout);

            auto vLayout = ftk::VerticalLayout::create(context);
            vLayout->setMarginRole(ftk::SizeRole::Margin);
            p.compareLayout = ftk::FormLayout::create(context, vLayout);
            p.compareLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.compareLayout->addRow("Mode:", p.compareComboBox);
            p.compareLayout->addRow("Time:", p.compareTimeComboBox);
            p.compareLayout->addRow("X:", p.wipeXSlider);
            p.compareLayout->addRow("Y:", p.wipeYSlider);
            p.compareLayout->addRow("Rotation:", p.wipeRotationSlider);
            p.compareLayout->addRow("Amount:", p.overlaySlider);
            p.bellows["Compare"] = ftk::Bellows::create(context, "Compare", layout);
            p.bellows["Compare"]->setWidget(vLayout);

            auto scrollWidget = ftk::ScrollWidget::create(context, ftk::ScrollType::Both);
            scrollWidget->setBorder(false);
            scrollWidget->setWidget(layout);
            _setWidget(scrollWidget);

            _loadSettings(p.bellows);

            auto appWeak = std::weak_ptr<App>(app);
            p.aButtonGroup->setCheckedCallback(
                [appWeak](int index, bool value)
                {
                    if (auto app = appWeak.lock())
                    {
                        app->getFilesModel()->setA(index);
                    }
                });

            p.bButtonGroup->setCheckedCallback(
                [appWeak](int index, bool value)
                {
                    if (auto app = appWeak.lock())
                    {
                        app->getFilesModel()->setB(index, value);
                    }
                });

            p.compareComboBox->setIndexCallback(
                [appWeak](int value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getFilesModel()->getCompareOptions();
                        options.compare = static_cast<tl::Compare>(value);
                        app->getFilesModel()->setCompareOptions(options);
                    }
                });

            p.compareTimeComboBox->setIndexCallback(
                [appWeak](int value)
                {
                    if (auto app = appWeak.lock())
                    {
                        app->getFilesModel()->setCompareTime(
                            static_cast<tl::CompareTime>(value));
                    }
                });

            p.wipeXSlider->setCallback(
                [appWeak](float value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getFilesModel()->getCompareOptions();
                        options.wipeCenter.x = value;
                        app->getFilesModel()->setCompareOptions(options);
                    }
                });

            p.wipeYSlider->setCallback(
                [appWeak](float value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getFilesModel()->getCompareOptions();
                        options.wipeCenter.y = value;
                        app->getFilesModel()->setCompareOptions(options);
                    }
                });

            p.wipeRotationSlider->setCallback(
                [appWeak](float value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getFilesModel()->getCompareOptions();
                        options.wipeRotation = value;
                        app->getFilesModel()->setCompareOptions(options);
                    }
                });

            p.overlaySlider->setCallback(
                [appWeak](float value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getFilesModel()->getCompareOptions();
                        options.overlay = value;
                        app->getFilesModel()->setCompareOptions(options);
                    }
                });

            p.filesObserver = ftk::ListObserver<std::shared_ptr<FilesModelItem> >::create(
                app->getFilesModel()->observeFiles(),
                [this](const std::vector<std::shared_ptr<FilesModelItem> >& value)
                {
                    _filesUpdate(value);
                });

            p.aObserver = ftk::Observer<std::shared_ptr<FilesModelItem> >::create(
                app->getFilesModel()->observeA(),
                [this](const std::shared_ptr<FilesModelItem>& value)
                {
                    _aUpdate(value);
                });

            p.bObserver = ftk::ListObserver<std::shared_ptr<FilesModelItem> >::create(
                app->getFilesModel()->observeB(),
                [this](const std::vector<std::shared_ptr<FilesModelItem> >& value)
                {
                    _bUpdate(value);
                });

            p.layersObserver = ftk::ListObserver<int>::create(
                app->getFilesModel()->observeLayers(),
                [this](const std::vector<int>& value)
                {
                    _layersUpdate(value);
                });

            p.compareObserver = ftk::Observer<tl::CompareOptions>::create(
                app->getFilesModel()->observeCompareOptions(),
                [this](const tl::CompareOptions& value)
                {
                    _compareUpdate(value);
                });

            p.compareTimeObserver = ftk::Observer<tl::CompareTime>::create(
                app->getFilesModel()->observeCompareTime(),
                [this](const tl::CompareTime& value)
                {
                    _p->compareTimeComboBox->setCurrentIndex(static_cast<int>(value));
                });
        }

        FilesTool::FilesTool() :
            _p(new Private)
        {}

        FilesTool::~FilesTool()
        {
            _saveSettings(_p->bellows);
        }

        std::shared_ptr<FilesTool> FilesTool::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<FilesTool>(new FilesTool);
            out->_init(context, app, parent);
            return out;
        }

        void FilesTool::_filesUpdate(const std::vector<std::shared_ptr<FilesModelItem> >& value)
        {
            FTK_P();
            p.aButtonGroup->clearButtons();
            p.bButtonGroup->clearButtons();
            p.layerComboBoxes.clear();
            auto children = p.widgetLayout->getChildren();
            for (const auto& widget : children)
            {
                widget->setParent(nullptr);
            }
            children.clear();
            auto appWeak = _app;
            if (auto app = appWeak.lock())
            {
                const auto& a = app->getFilesModel()->getA();
                const auto& b = app->getFilesModel()->getB();
                if (auto context = getContext())
                {
                    size_t row = 0;
                    for (const auto& item : value)
                    {
                        auto aButton = FileButton::create(context, item);
                        aButton->setChecked(item == a);
                        aButton->setTooltip(item->path.get());
                        p.aButtons[item] = aButton;
                        p.aButtonGroup->addButton(aButton);
                        aButton->setParent(p.widgetLayout);
                        p.widgetLayout->setGridPos(aButton, row, 0);

                        auto bButton = ftk::ToolButton::create(context);
                        bButton->setText("B");
                        const auto i = std::find(b.begin(), b.end(), item);
                        bButton->setChecked(i != b.end());
                        bButton->setVAlign(ftk::VAlign::Center);
                        bButton->setTooltip("Set the B file(s).");
                        p.bButtons[item] = bButton;
                        p.bButtonGroup->addButton(bButton);
                        bButton->setParent(p.widgetLayout);
                        p.widgetLayout->setGridPos(bButton, row, 1);

                        auto layerComboBox = ftk::ComboBox::create(context);
                        layerComboBox->setItems(item->videoLayers);
                        layerComboBox->setCurrentIndex(item->videoLayer);
                        layerComboBox->setVAlign(ftk::VAlign::Center);
                        layerComboBox->setTooltip("Set the current layer.");
                        p.layerComboBoxes.push_back(layerComboBox);
                        layerComboBox->setParent(p.widgetLayout);
                        p.widgetLayout->setGridPos(layerComboBox, row, 2);

                        layerComboBox->setIndexCallback(
                            [appWeak, item](int value)
                            {
                                if (auto app = appWeak.lock())
                                {
                                    app->getFilesModel()->setLayer(item, value);
                                }
                            });

                        ++row;
                    }
                    if (value.empty())
                    {
                        auto label = ftk::Label::create(context, "No files open", p.widgetLayout);
                        p.widgetLayout->setGridPos(label, 0, 0);
                    }
                }
            }
        }

        void FilesTool::_aUpdate(const std::shared_ptr<FilesModelItem>& value)
        {
            FTK_P();
            for (const auto& button : p.aButtons)
            {
                button.second->setChecked(button.first == value);
            }
        }

        void FilesTool::_bUpdate(const std::vector<std::shared_ptr<FilesModelItem> >& value)
        {
            FTK_P();
            for (const auto& button : p.bButtons)
            {
                const auto i = std::find(value.begin(), value.end(), button.first);
                button.second->setChecked(i != value.end());
            }
        }

        void FilesTool::_layersUpdate(const std::vector<int>& value)
        {
            FTK_P();
            for (size_t i = 0; i < value.size() && i < p.layerComboBoxes.size(); ++i)
            {
                p.layerComboBoxes[i]->setCurrentIndex(value[i]);
            }
        }

        void FilesTool::_compareUpdate(const tl::CompareOptions& value)
        {
            FTK_P();
            p.compareComboBox->setCurrentIndex(static_cast<int>(value.compare));
            p.wipeXSlider->setValue(value.wipeCenter.x);
            p.wipeYSlider->setValue(value.wipeCenter.y);
            p.wipeRotationSlider->setValue(value.wipeRotation);
            p.overlaySlider->setValue(value.overlay);

            p.compareLayout->setRowVisible(p.wipeXSlider, value.compare == tl::Compare::Wipe);
            p.compareLayout->setRowVisible(p.wipeYSlider, value.compare == tl::Compare::Wipe);
            p.compareLayout->setRowVisible(p.wipeRotationSlider, value.compare == tl::Compare::Wipe);
            p.compareLayout->setRowVisible(p.overlaySlider, value.compare == tl::Compare::Overlay);
        }
    }
}
