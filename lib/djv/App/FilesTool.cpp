// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/FilesTool.h>

#include <djv/UI/FrameRangePopup.h>

#include <djv/Models/FilesModel.h>

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
#include <ftk/Core/Format.h>
#include <ftk/UI/Label.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/ScreenshotTag.h>
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
                std::shared_ptr<ui::FileThumbnail> thumbnail;\
                std::shared_ptr<ftk::ToolButton> nameButton;
                std::shared_ptr<ftk::ToolButton> bButton;
                std::shared_ptr<ftk::ComboBox> layerComboBox;
                std::shared_ptr<ftk::ToolButton> rangeButton;
            };
        }

        struct FilesTool::Private
        {
            std::shared_ptr<ftk::Settings> settings;

            std::shared_ptr<ui::FrameRangePopup> rangePopup;
            std::shared_ptr<ftk::ButtonGroup> aButtonGroup;
            std::shared_ptr<ftk::ButtonGroup> bButtonGroup;
            std::vector<FileWidget> widgets;
            std::shared_ptr<ftk::ComboBox> compareComboBox;
            std::shared_ptr<ftk::FloatEditSlider> wipeXSlider;
            std::shared_ptr<ftk::FloatEditSlider> wipeYSlider;
            std::shared_ptr<ftk::FloatEditSlider> wipeRotationSlider;
            std::shared_ptr<ftk::FloatEditSlider> overlaySlider;
            std::shared_ptr<ftk::ComboBox> compareTimeComboBox;
            std::shared_ptr<ftk::CheckBox> sameSizeCheckBox;
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

            p.compareTimeComboBox = ftk::ComboBox::create(
                context,
                models::getCompareTimeLabels());
            p.compareTimeComboBox->setTooltip(
                "Which frame of each file is shown together: the same frame\n"
                "counted from the start of each, or the same timecode.");
            p.compareTimeComboBox->setHStretch(ftk::Stretch::Expanding);
            ftk::setScreenshotTag(p.compareTimeComboBox, "Files.CompareTime");

            p.sameSizeCheckBox = ftk::CheckBox::create(context);
            p.sameSizeCheckBox->setHStretch(ftk::Stretch::Expanding);
            p.sameSizeCheckBox->setTooltip(
                "Draw the compared files at the size of the current file,\n"
                "so a smaller one is not shown tiny beside it.");

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
            p.compareLayout->addRow("X:", p.wipeXSlider);
            p.compareLayout->addRow("Y:", p.wipeYSlider);
            p.compareLayout->addRow("Rotation:", p.wipeRotationSlider);
            p.compareLayout->addRow("Amount:", p.overlaySlider);
            p.compareLayout->addRow("Sync by:", p.compareTimeComboBox);
            p.compareLayout->addRow("Same size:", p.sameSizeCheckBox);
            p.bellows["Compare"] = ftk::Bellows::create(context, "Compare", layout);
            p.bellows["Compare"]->setWidget(vLayout);

            _setWidget(layout);

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

            p.compareTimeComboBox->setIndexCallback(
                [appWeak](int value)
                {
                    if (auto app = appWeak.lock())
                    {
                        app->getFilesModel()->setCompareTime(
                            static_cast<tl::CompareTime>(value));
                    }
                });

            p.sameSizeCheckBox->setCheckedCallback(
                [appWeak](bool value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getFilesModel()->getCompareOptions();
                        options.sameSize = value;
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

                        widget.nameButton = ftk::ToolButton::create(
                            context,
                            ftk::elide(item->path.getFileName(), 24),
                            p.widgetLayout);
                        widget.nameButton->setChecked(item == a);
                        widget.nameButton->setHStretch(ftk::Stretch::Expanding);
                        widget.nameButton->setVAlign(ftk::VAlign::Center);
                        widget.nameButton->setTooltip(
                            item->path.get() + "\n\nSet the A file.");
                        p.aButtonGroup->addButton(widget.nameButton);
                        p.widgetLayout->setGridPos(widget.nameButton, row, 1);

                        widget.bButton = ftk::ToolButton::create(context, "B", p.widgetLayout);
                        const auto i = std::find(b.begin(), b.end(), item);
                        widget.bButton->setChecked(i != b.end());
                        widget.bButton->setVAlign(ftk::VAlign::Center);
                        widget.bButton->setTooltip("Set the B file(s).");
                        p.bButtonGroup->addButton(widget.bButton);
                        p.widgetLayout->setGridPos(widget.bButton, row, 2);

                        widget.layerComboBox = ftk::ComboBox::create(context, p.widgetLayout);
                        widget.layerComboBox->setItems(item->videoLayers);
                        widget.layerComboBox->setCurrentIndex(item->videoLayer);
                        widget.layerComboBox->setVAlign(ftk::VAlign::Center);
                        widget.layerComboBox->setTooltip("Set the current layer.");
                        // Layer names can be long -- and are, in a multi part
                        // EXR -- and the column is as wide as the longest one
                        // in it. The menu still shows them whole.
                        // Kept from the end: layer names share a prefix and
                        // differ where they finish.
                        widget.layerComboBox->setElide(12, ftk::ElideMode::Left);
                        // A file with one layer has nothing to choose, and the
                        // column is as wide as the longest layer name in it.
                        widget.layerComboBox->setVisible(
                            item->videoLayers.size() > 1);
                        p.widgetLayout->setGridPos(widget.layerComboBox, row, 3);

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
                        // which need not be what is on disk yet. It is set
                        // rarely, so the row shows it and the editing is in a
                        // popup rather than two edits in every row.
                        if (item->path.hasNum() && item->path.testExt(seqExts))
                        {
                            // What the file turned out to be when it opened.
                            // The path only knows the range once one has been
                            // stated for it; until then it names one file.
                            ftk::RangeI64 range(0, 0);
                            if (item->timeRange.has_value())
                            {
                                const int64_t start = static_cast<int64_t>(
                                    item->timeRange->start_time().value());
                                range = ftk::RangeI64(
                                    start,
                                    start + static_cast<int64_t>(
                                        item->timeRange->duration().value()) - 1);
                            }
                            else if (item->path.getFrames().has_value())
                            {
                                range = item->path.getFrames().value();
                            }

                            widget.rangeButton = ftk::ToolButton::create(
                                context,
                                ftk::Format("{0}-{1}").
                                    arg(range.min()).arg(range.max()),
                                p.widgetLayout);
                            widget.rangeButton->setVAlign(ftk::VAlign::Center);
                            widget.rangeButton->setTooltip(
                                "The frame range of the sequence.");
                            p.widgetLayout->setGridPos(widget.rangeButton, row, 4);

                            auto buttonWeak =
                                std::weak_ptr<ftk::ToolButton>(widget.rangeButton);
                            widget.rangeButton->setClickedCallback(
                                [this, item, range, buttonWeak]
                                {
                                    _showRangePopup(item, range, buttonWeak.lock());
                                });
                        }

                        p.widgets.push_back(widget);

                        if (0 == row)
                        {
                            ftk::setScreenshotTag(
                                widget.layerComboBox,
                                "Files.CurrentLayer");
                            ftk::setScreenshotTag(
                                widget.rangeButton,
                                "Files.FrameRange");

                            auto spacer = ftk::Spacer::create(context, ftk::Orientation::Horizontal, p.widgetLayout);
                            spacer->setSpacingRole(ftk::SizeRole::SpacingTool);
                            p.widgetLayout->setGridPos(spacer, 0, 5);
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

        void FilesTool::_showRangePopup(
            const std::shared_ptr<models::FilesModelItem>& item,
            const ftk::RangeI64& range,
            const std::shared_ptr<ftk::IWidget>& button)
        {
            FTK_P();
            if (p.rangePopup || !button)
                return;
            if (auto context = getContext())
            {
                p.rangePopup = ui::FrameRangePopup::create(context, range);
                auto buttonWeak = std::weak_ptr<ftk::IWidget>(button);
                p.rangePopup->setCallback(
                    [this, item, buttonWeak](const ftk::RangeI64& value)
                    {
                        _rangeUpdate(item, value);
                    });
                p.rangePopup->open(getWindow(), button->getGeometry());
                p.rangePopup->setCloseCallback(
                    [this]
                    {
                        _p->rangePopup.reset();
                    });
            }
        }

        void FilesTool::_aUpdate(const std::shared_ptr<models::FilesModelItem>& value)
        {
            FTK_P();
            for (const auto& i : p.widgets)
            {
                i.nameButton->setChecked(i.item == value);
                ftk::setScreenshotTag(i.nameButton, i.item == value ? "Files.CurrentFile" : "");
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
            p.sameSizeCheckBox->setChecked(value.sameSize);

            p.compareLayout->setRowVisible(p.wipeXSlider, value.compare == tl::Compare::Wipe);
            p.compareLayout->setRowVisible(p.wipeYSlider, value.compare == tl::Compare::Wipe);
            p.compareLayout->setRowVisible(p.wipeRotationSlider, value.compare == tl::Compare::Wipe);
            p.compareLayout->setRowVisible(p.overlaySlider, value.compare == tl::Compare::Overlay);
        }
    }
}
