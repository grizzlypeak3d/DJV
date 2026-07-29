// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/FilesTool.h>

#include <djv/App/App.h>
#include <djv/UI/FileThumbnail.h>
#include <djv/Models/SettingsModel.h>

#include <tlRender/Timeline/Util.h>

#include <ftk/UI/Bellows.h>
#include <ftk/UI/ButtonGroup.h>
#include <ftk/UI/CheckBox.h>
#include <ftk/UI/ComboBox.h>
#include <ftk/UI/Divider.h>
#include <ftk/UI/FloatEditSlider.h>
#include <ftk/UI/FormLayout.h>
#include <ftk/UI/GridLayout.h>
#include <ftk/UI/IntEdit.h>
#include <ftk/UI/Label.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/ScreenshotTag.h>
#include <ftk/UI/ScrollWidget.h>
#include <ftk/UI/Spacer.h>

#include <ftk/Core/Timer.h>
#include <ftk/UI/Settings.h>
#include <ftk/UI/ToolButton.h>

namespace djv
{
    namespace app
    {
        namespace
        {
            struct FileWidget
            {
                std::shared_ptr<models::FilesModelItem> item;
                std::shared_ptr<ui::FileThumbnail> thumbnail;
                std::shared_ptr<ftk::Label> label;
                std::shared_ptr<ftk::ToolButton> aButton;
                std::shared_ptr<ftk::ToolButton> bButton;
                std::shared_ptr<ftk::ComboBox> layerComboBox;
                std::shared_ptr<ftk::IntEdit> frameStartEdit;
                std::shared_ptr<ftk::IntEdit> frameEndEdit;
            };
        }

        struct FilesTool::Private
        {
            std::shared_ptr<ftk::Settings> settings;

            std::shared_ptr<ftk::ButtonGroup> aButtonGroup;
            std::shared_ptr<ftk::ButtonGroup> bButtonGroup;
            std::vector<FileWidget> widgets;
            std::shared_ptr<ftk::ComboBox> compareComboBox;
            std::shared_ptr<ftk::ComboBox> compareTimeComboBox;
            std::shared_ptr<ftk::FloatEditSlider> wipeXSlider;
            std::shared_ptr<ftk::FloatEditSlider> wipeYSlider;
            std::shared_ptr<ftk::FloatEditSlider> wipeRotationSlider;
            std::shared_ptr<ftk::FloatEditSlider> overlaySlider;
            std::shared_ptr<ftk::CheckBox> fitToACheckBox;
            std::shared_ptr<ftk::FormLayout> compareLayout;
            std::map<std::string, std::shared_ptr<ftk::Bellows> > bellows;
            std::shared_ptr<ftk::GridLayout> widgetLayout;

            // Every step of a spin box is a value change, and applying a
            // range reopens the file, so the edits are let go of before the
            // range is acted on.
            std::shared_ptr<ftk::Timer> rangeTimer;
            std::shared_ptr<models::FilesModelItem> rangeItem;
            ftk::RangeI64 rangeValue;

            std::shared_ptr<ftk::ListObserver<std::shared_ptr<models::FilesModelItem> > > filesObserver;
            std::shared_ptr<ftk::Observer<std::shared_ptr<models::FilesModelItem> > > aObserver;
            std::shared_ptr<ftk::ListObserver<std::shared_ptr<models::FilesModelItem> > > bObserver;
            std::shared_ptr<ftk::ListObserver<int> > layersObserver;
            std::shared_ptr<ftk::Observer<tl::CompareOptions> > compareObserver;
            std::shared_ptr<ftk::Observer<tl::CompareTime> > compareTimeObserver;
        };

        void FilesTool::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<MainWindow>& mainWindow,
            const std::shared_ptr<IWidget>& parent)
        {
            IToolWidget::_init(
                context,
                app,
                mainWindow,
                "Files",
                "Files",
                "djv::app::FilesTool",
                parent);
            FTK_P();

            p.settings = app->getSettings();

            p.rangeTimer = ftk::Timer::create(context);

            p.aButtonGroup = ftk::ButtonGroup::create(context, ftk::ButtonGroupType::Radio);
            p.bButtonGroup = ftk::ButtonGroup::create(context, ftk::ButtonGroupType::Check);

            p.compareComboBox = ftk::ComboBox::create(
                context,
                tl::getCompareLabels());
            p.compareComboBox->setHStretch(ftk::Stretch::Expanding);
            ftk::setScreenshotTag(p.compareComboBox, "Files.CompareMode");

            p.compareTimeComboBox = ftk::ComboBox::create(
                context,
                tl::getCompareTimeLabels());
            p.compareTimeComboBox->setHStretch(ftk::Stretch::Expanding);
            ftk::setScreenshotTag(p.compareTimeComboBox, "Files.CompareTime");

            p.wipeXSlider = ftk::FloatEditSlider::create(context);
            p.wipeXSlider->setDefault(.5F);
            ftk::setScreenshotTag(p.wipeXSlider, "Files.CompareOptions");
            p.wipeYSlider = ftk::FloatEditSlider::create(context);
            p.wipeYSlider->setDefault(.5F);
            p.wipeRotationSlider = ftk::FloatEditSlider::create(context);
            p.wipeRotationSlider->setRange(0.F, 360.F);
            p.wipeRotationSlider->setStep(1.F);
            p.wipeRotationSlider->setLargeStep(10.F);
            p.wipeRotationSlider->setDefault(0.F);

            p.overlaySlider = ftk::FloatEditSlider::create(context);
            p.overlaySlider->setDefault(.5F);

            p.fitToACheckBox = ftk::CheckBox::create(context);
            p.fitToACheckBox->setHStretch(ftk::Stretch::Expanding);

            auto layout = ftk::VerticalLayout::create(context);
            layout->setSpacingRole(ftk::SizeRole::None);

            p.widgetLayout = ftk::GridLayout::create(context, layout);
            p.widgetLayout->setSpacingRole(
                ftk::SizeRole::SpacingSmall,
                ftk::SizeRole::None);
            p.widgetLayout->setRowBackgroundRole(ftk::ColorRole::Header);

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
            p.compareLayout->addRow("Fit to A:", p.fitToACheckBox);
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

            p.fitToACheckBox->setCheckedCallback(
                [appWeak](bool value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getFilesModel()->getCompareOptions();
                        options.fitToA = value;
                        app->getFilesModel()->setCompareOptions(options);
                    }
                });

            p.filesObserver = ftk::ListObserver<std::shared_ptr<models::FilesModelItem> >::create(
                app->getFilesModel()->observeFiles(),
                [this](const std::vector<std::shared_ptr<models::FilesModelItem> >& value)
                {
                    _filesUpdate(value);
                });

            p.aObserver = ftk::Observer<std::shared_ptr<models::FilesModelItem> >::create(
                app->getFilesModel()->observeA(),
                [this](const std::shared_ptr<models::FilesModelItem>& value)
                {
                    _aUpdate(value);
                });

            p.bObserver = ftk::ListObserver<std::shared_ptr<models::FilesModelItem> >::create(
                app->getFilesModel()->observeB(),
                [this](const std::vector<std::shared_ptr<models::FilesModelItem> >& value)
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
            const std::shared_ptr<MainWindow>& mainWindow,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<FilesTool>(new FilesTool);
            out->_init(context, app, mainWindow, parent);
            return out;
        }

        void FilesTool::_rangeUpdate(
            const std::shared_ptr<models::FilesModelItem>& item,
            const ftk::RangeI64& value)
        {
            FTK_P();
            p.rangeItem = item;
            p.rangeValue = value;

            // Restarted on each change, so holding a spin box down reopens
            // the file once, at the range it is left on, rather than at every
            // value passed through on the way there.
            p.rangeTimer->start(
                std::chrono::milliseconds(500),
                [this]
                {
                    FTK_P();
                    if (p.rangeItem)
                    {
                        if (auto app = _app.lock())
                        {
                            app->getFilesModel()->setFrames(
                                p.rangeItem, p.rangeValue);
                        }
                        p.rangeItem.reset();
                    }
                });
        }

        void FilesTool::_filesUpdate(const std::vector<std::shared_ptr<models::FilesModelItem> >& value)
        {
            FTK_P();
            p.widgets.clear();
            p.aButtonGroup->clearButtons();
            p.bButtonGroup->clearButtons();
            p.widgetLayout->clear();
            auto appWeak = _app;
            if (auto app = appWeak.lock())
            {
                const auto& a = app->getFilesModel()->getA();
                const auto& b = app->getFilesModel()->getB();
                if (auto context = getContext())
                {
                    const std::vector<std::string> seqExts = tl::getExts(
                        context, static_cast<int>(tl::FileType::Seq));
                    size_t row = 0;
                    for (const auto& item : value)
                    {
                        FileWidget widget;
                        widget.item = item;

                        widget.thumbnail = ui::FileThumbnail::create(
                            context,
                            item,
                            app->getSettingsModel()->getIOOptions(),
                            p.widgetLayout);
                        p.widgetLayout->setGridPos(widget.thumbnail, row, 0);

                        widget.label = ftk::Label::create(
                            context,
                            ftk::elide(item->path.getFileName(), 24),
                            p.widgetLayout);
                        widget.label->setMarginRole(ftk::SizeRole::MarginSmall);
                        widget.label->setHStretch(ftk::Stretch::Expanding);
                        widget.label->setVAlign(ftk::VAlign::Center);
                        widget.label->setTooltip(item->path.get());
                        p.widgetLayout->setGridPos(widget.label, row, 1);

                        widget.aButton = ftk::ToolButton::create(context, "A", p.widgetLayout);
                        widget.aButton->setChecked(item == a);
                        widget.aButton->setVAlign(ftk::VAlign::Center);
                        widget.aButton->setTooltip("Set the A file.");
                        p.aButtonGroup->addButton(widget.aButton);
                        p.widgetLayout->setGridPos(widget.aButton, row, 2);

                        widget.bButton = ftk::ToolButton::create(context, "B", p.widgetLayout);
                        const auto i = std::find(b.begin(), b.end(), item);
                        widget.bButton->setChecked(i != b.end());
                        widget.bButton->setVAlign(ftk::VAlign::Center);
                        widget.bButton->setTooltip("Set the B file(s).");
                        p.bButtonGroup->addButton(widget.bButton);
                        p.widgetLayout->setGridPos(widget.bButton, row, 3);

                        widget.layerComboBox = ftk::ComboBox::create(context, p.widgetLayout);
                        widget.layerComboBox->setItems(item->videoLayers);
                        widget.layerComboBox->setCurrentIndex(item->videoLayer);
                        widget.layerComboBox->setVAlign(ftk::VAlign::Center);
                        widget.layerComboBox->setTooltip("Set the current layer.");
                        p.widgetLayout->setGridPos(widget.layerComboBox, row, 4);

                        widget.layerComboBox->setIndexCallback(
                            [appWeak, item](int value)
                            {
                                if (auto app = appWeak.lock())
                                {
                                    app->getFilesModel()->setLayer(item, value);
                                }
                            });

                        // Only an image sequence has a frame range to state.
                        // The range is what the sequence is meant to cover,
                        // which need not be what is on disk yet.
                        if (item->path.hasNum() && item->path.testExt(seqExts))
                        {
                            auto rangeLayout = ftk::HorizontalLayout::create(
                                context, p.widgetLayout);
                            rangeLayout->setSpacingRole(ftk::SizeRole::SpacingTool);
                            rangeLayout->setVAlign(ftk::VAlign::Center);
                            p.widgetLayout->setGridPos(rangeLayout, row, 5);

                            widget.frameStartEdit = ftk::IntEdit::create(
                                context, rangeLayout);
                            widget.frameEndEdit = ftk::IntEdit::create(
                                context, rangeLayout);
                            widget.frameStartEdit->setRange(-999999, 999999);
                            widget.frameEndEdit->setRange(-999999, 999999);
                            // What the file turned out to be when it opened.
                            // The path only knows the range once one has been
                            // stated for it; until then it names one file.
                            int start = 0;
                            int end = 0;
                            if (!tl::compareExact(item->timeRange, tl::invalidTimeRange))
                            {
                                start = static_cast<int>(
                                    item->timeRange.start_time().value());
                                end = start + static_cast<int>(
                                    item->timeRange.duration().value()) - 1;
                            }
                            else if (item->path.getFrames().has_value())
                            {
                                const ftk::RangeI64& frames =
                                    item->path.getFrames().value();
                                start = static_cast<int>(frames.min());
                                end = static_cast<int>(frames.max());
                            }
                            widget.frameStartEdit->setValue(start);
                            widget.frameEndEdit->setValue(end);
                            widget.frameStartEdit->setTooltip(
                                "Start frame of the sequence.\n"
                                "\n"
                                "Frames that are not there yet follow the\n"
                                "missing frames setting, so a render can be\n"
                                "opened over the range it will end up with.");
                            widget.frameEndEdit->setTooltip(
                                "End frame of the sequence.");

                            // Each holds the other's end of the range, so
                            // neither can be taken past it. A range that runs
                            // backwards is silently turned around when it is
                            // built, which leaves the two edits showing
                            // something the file does not have.
                            widget.frameStartEdit->setRange(
                                -999999, widget.frameEndEdit->getValue());
                            widget.frameEndEdit->setRange(
                                widget.frameStartEdit->getValue(), 999999);

                            // Weak, so the two edits do not keep each other
                            // alive through their callbacks.
                            std::weak_ptr<ftk::IntEdit> startWeak =
                                widget.frameStartEdit;
                            std::weak_ptr<ftk::IntEdit> endWeak =
                                widget.frameEndEdit;
                            widget.frameStartEdit->setCallback(
                                [this, item, endWeak](int value)
                                {
                                    if (auto end = endWeak.lock())
                                    {
                                        end->setRange(value, 999999);
                                        _rangeUpdate(
                                            item,
                                            ftk::RangeI64(value, end->getValue()));
                                    }
                                });
                            widget.frameEndEdit->setCallback(
                                [this, item, startWeak](int value)
                                {
                                    if (auto start = startWeak.lock())
                                    {
                                        start->setRange(-999999, value);
                                        _rangeUpdate(
                                            item,
                                            ftk::RangeI64(start->getValue(), value));
                                    }
                                });
                        }

                        p.widgets.push_back(widget);

                        if (0 == row)
                        {
                            ftk::setScreenshotTag(widget.layerComboBox, "Files.CurrentLayer");
                            if (widget.frameStartEdit)
                            {
                                ftk::setScreenshotTag(
                                    widget.frameStartEdit, "Files.FrameRange");
                            }

                            auto spacer = ftk::Spacer::create(context, ftk::Orientation::Horizontal, p.widgetLayout);
                            spacer->setSpacingRole(ftk::SizeRole::SpacingTool);
                            p.widgetLayout->setGridPos(spacer, 0, 6);
                        }
                        ++row;
                    }
                    if (value.empty())
                    {
                        auto label = ftk::Label::create(context, "No files open", p.widgetLayout);
                        label->setMarginRole(ftk::SizeRole::Margin);
                        p.widgetLayout->setGridPos(label, 0, 0);
                    }
                }
            }
        }

        void FilesTool::_aUpdate(const std::shared_ptr<models::FilesModelItem>& value)
        {
            FTK_P();
            for (const auto& i : p.widgets)
            {
                i.aButton->setChecked(i.item == value);
                ftk::setScreenshotTag(i.aButton, i.item == value ? "Files.CurrentFile" : "");
            }
        }

        void FilesTool::_bUpdate(const std::vector<std::shared_ptr<models::FilesModelItem> >& value)
        {
            FTK_P();
            for (const auto& i : p.widgets)
            {
                const auto j = std::find(value.begin(), value.end(), i.item);
                i.bButton->setChecked(j != value.end());
                ftk::setScreenshotTag(i.bButton, j != value.end() ? "Files.BFile" : "");
            }
        }

        void FilesTool::_layersUpdate(const std::vector<int>& value)
        {
            FTK_P();
            for (size_t i = 0; i < value.size() && i < p.widgets.size(); ++i)
            {
                p.widgets[i].layerComboBox->setCurrentIndex(value[i]);
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
            p.fitToACheckBox->setChecked(value.fitToA);

            p.compareLayout->setRowVisible(p.wipeXSlider, value.compare == tl::Compare::Wipe);
            p.compareLayout->setRowVisible(p.wipeYSlider, value.compare == tl::Compare::Wipe);
            p.compareLayout->setRowVisible(p.wipeRotationSlider, value.compare == tl::Compare::Wipe);
            p.compareLayout->setRowVisible(p.overlaySlider, value.compare == tl::Compare::Overlay);
        }
    }
}
