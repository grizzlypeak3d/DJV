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
#include <ftk/UI/ItemButtonList.h>
#include <ftk/UI/IntEdit.h>
#include <ftk/Core/Format.h>
#include <ftk/UI/Label.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/ScreenshotTag.h>
#include <ftk/UI/Spacer.h>

#include <ftk/Core/Timer.h>
#include <ftk/UI/Settings.h>
#include <ftk/UI/ToolButton.h>

#include <algorithm>

namespace djv
{
    namespace app
    {
        namespace
        {
            struct FileWidget
            {
                std::shared_ptr<models::FilesModelItem> item;
                std::shared_ptr<ftk::ItemButton> button;
                std::shared_ptr<ui::FileThumbnail> thumbnail;
                std::shared_ptr<ftk::ToolButton> bButton;
                std::shared_ptr<ftk::ComboBox> layerComboBox;
                std::shared_ptr<ftk::ToolButton> rangeButton;
            };
        }

        struct FilesTool::Private
        {
            std::shared_ptr<ftk::Settings> settings;

            std::shared_ptr<ui::FrameRangePopup> rangePopup;
            std::shared_ptr<ftk::ButtonGroup> bButtonGroup;
            std::vector<FileWidget> widgets;
            std::shared_ptr<ftk::ComboBox> compareComboBox;
            std::shared_ptr<ftk::FloatEditSlider> wipeXSlider;
            std::shared_ptr<ftk::FloatEditSlider> wipeYSlider;
            std::shared_ptr<ftk::FloatEditSlider> wipeRotationSlider;
            std::shared_ptr<ftk::FloatEditSlider> overlaySlider;
            std::shared_ptr<ftk::FloatEditSlider> differenceGainSlider;
            std::shared_ptr<ftk::ComboBox> compareTimeComboBox;
            std::shared_ptr<ftk::CheckBox> sameSizeCheckBox;
            std::shared_ptr<ftk::FormLayout> compareLayout;
            std::map<std::string, std::shared_ptr<ftk::Bellows> > bellows;
            std::shared_ptr<ftk::ItemButtonList> fileList;

            // Where a dragged file would land: an index into the rows,
            // counting the gap after the last one, or -1 for nowhere.
            int dropTarget = -1;
            int handle = 0;

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

            p.differenceGainSlider = ftk::FloatEditSlider::create(context);
            p.differenceGainSlider->setRange(1.F, 32.F);
            p.differenceGainSlider->setStep(1.F);
            p.differenceGainSlider->setLargeStep(4.F);
            p.differenceGainSlider->setDefault(1.F);
            p.differenceGainSlider->getModel()->setRangeSoft(true);
            p.differenceGainSlider->setTooltip(
                "Multiply the difference, so that a small one can be seen.\n"
                "A compressed file differs from its source by a code value\n"
                "or two, which is not distinguishable from black on its own.");

            p.compareTimeComboBox = ftk::ComboBox::create(
                context,
                models::getCompareTimeLabels());
            p.compareTimeComboBox->setTooltip(
                "Which frame of each file is shown together: the same frame\n"
                "counted from the start of each, or the same timecode.\n"
                "Files whose timecodes do not overlap have nothing to show\n"
                "together.");
            p.compareTimeComboBox->setHStretch(ftk::Stretch::Expanding);
            ftk::setScreenshotTag(p.compareTimeComboBox, "Files.CompareTime");

            p.sameSizeCheckBox = ftk::CheckBox::create(context);
            p.sameSizeCheckBox->setHStretch(ftk::Stretch::Expanding);
            p.sameSizeCheckBox->setTooltip(
                "Draw the compared files at the size of the current file,\n"
                "so a smaller one is not shown tiny beside it.");

            auto layout = ftk::VerticalLayout::create(context);
            layout->setSpacingRole(ftk::SizeRole::None);

            // The files are a list of items, like the markers and the file
            // browser: the list is the focus unit, the arrows browse it,
            // Return sets "A", Delete closes, and the whole row drags.
            p.fileList = ftk::ItemButtonList::create(context, layout);
            p.fileList->setSpacingRole(ftk::SizeRole::None);

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
            p.compareLayout->addRow("Gain:", p.differenceGainSlider);
            p.compareLayout->addRow("Sync by:", p.compareTimeComboBox);
            p.compareLayout->addRow("Same size:", p.sameSizeCheckBox);
            p.bellows["Compare"] = ftk::Bellows::create(context, "Compare", layout);
            p.bellows["Compare"]->setWidget(vLayout);

            _setWidget(layout);

            _loadSettings(p.bellows);

            auto appWeak = std::weak_ptr<App>(app);
            p.fileList->setActivateCallback(
                [appWeak](int index)
                {
                    if (auto app = appWeak.lock())
                    {
                        app->getFilesModel()->setA(index);
                    }
                });

            p.fileList->setDeleteCallback(
                [appWeak](int index)
                {
                    if (auto app = appWeak.lock())
                    {
                        app->getFilesModel()->close(index);
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

            p.differenceGainSlider->setCallback(
                [appWeak](float value)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto options = app->getFilesModel()->getCompareOptions();
                        options.differenceGain = value;
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

            // The same files in a different order: move the rows rather than
            // rebuilding them. Rebuilding makes new thumbnails and lays out
            // an empty grid along the way, which loses the scroll position,
            // so reordering a long list would jump. The same files in the
            // same order fall through to the rebuild -- that is refresh(),
            // announcing that what the items hold has changed.
            if (!value.empty() && value.size() == p.widgets.size())
            {
                std::vector<FileWidget> widgets;
                bool reordered = false;
                for (size_t i = 0; i < value.size(); ++i)
                {
                    const auto j = std::find_if(
                        p.widgets.begin(),
                        p.widgets.end(),
                        [&value, i](const FileWidget& w)
                        {
                            return w.item == value[i];
                        });
                    if (j == p.widgets.end())
                    {
                        break;
                    }
                    widgets.push_back(*j);
                    reordered |= p.widgets[i].item != value[i];
                }
                if (widgets.size() == value.size() && reordered)
                {
                    p.widgets = widgets;
                    // The button group answers clicks with an index in the
                    // order the buttons were added, so it has to follow
                    // the new order or every click selects the old row.
                    p.bButtonGroup->clearButtons();
                    for (size_t row = 0; row < p.widgets.size(); ++row)
                    {
                        auto& widget = p.widgets[row];
                        p.bButtonGroup->addButton(widget.bButton);
                        ftk::setScreenshotTag(
                            widget.thumbnail,
                            ftk::Format("Files.FileThumbnail{0}").arg(row));
                        p.fileList->moveToIndex(widget.button, row);
                    }
                    return;
                }
            }

            p.widgets.clear();
            p.bButtonGroup->clearButtons();
            p.fileList->clear();
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

                        // The row is one item: click or Return sets "A",
                        // and a press that moves drags the file to a new
                        // place in the list.
                        widget.button = ftk::ItemButton::create(
                            context, p.fileList);
                        widget.button->setAcceptsKeyFocus(false);
                        widget.button->setChecked(item == a);
                        widget.button->setTooltip(
                            item->path.get() + "\n\nSet the A file.");
                        ftk::setScreenshotTag(
                            widget.button,
                            item == a ? "Files.CurrentFile" : "");

                        auto rowLayout = ftk::HorizontalLayout::create(context);
                        rowLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);

                        widget.thumbnail = ui::FileThumbnail::create(
                            context,
                            item,
                            app->getSettingsModel()->getIOOptions(),
                            rowLayout);
                        ftk::setScreenshotTag(
                            widget.thumbnail,
                            ftk::Format("Files.FileThumbnail{0}").arg(row));

                        auto nameLabel = ftk::Label::create(
                            context,
                            ftk::elide(item->path.getFileName(), 24),
                            rowLayout);
                        nameLabel->setHStretch(ftk::Stretch::Expanding);
                        nameLabel->setVAlign(ftk::VAlign::Center);

                        widget.button->setClickedCallback(
                            [appWeak, item]
                            {
                                if (auto app = appWeak.lock())
                                {
                                    // By the item rather than a captured
                                    // index: the rows move.
                                    const auto& files =
                                        app->getFilesModel()->getFiles();
                                    const auto i = std::find(
                                        files.begin(), files.end(), item);
                                    if (i != files.end())
                                    {
                                        app->getFilesModel()->setA(
                                            static_cast<int>(i - files.begin()));
                                    }
                                }
                            });

                        widget.button->setDragDropDataCallback(
                            [item]
                            {
                                return std::make_shared<ui::FileDragDropData>(item);
                            });
                        auto thumbnailWeak =
                            std::weak_ptr<ui::FileThumbnail>(widget.thumbnail);
                        widget.button->setDragDropCursorCallback(
                            [thumbnailWeak]() -> std::shared_ptr<ftk::Image>
                            {
                                auto thumbnail = thumbnailWeak.lock();
                                return thumbnail ?
                                    thumbnail->getThumbnail() :
                                    nullptr;
                            });

                        // The controls keep the mouse to themselves: the
                        // gaps between them go quiet instead of flashing the
                        // row's hover, and a click that misses a control
                        // does nothing rather than setting "A".
                        auto controls = ftk::ItemControls::create(
                            context, rowLayout);
                        auto controlsLayout =
                            ftk::HorizontalLayout::create(context);
                        controlsLayout->setSpacingRole(
                            ftk::SizeRole::SpacingSmall);
                        controls->setWidget(controlsLayout);

                        widget.layerComboBox = ftk::ComboBox::create(context, controlsLayout);
                        widget.layerComboBox->setItems(item->videoLayers);
                        widget.layerComboBox->setCurrentIndex(item->videoLayer);
                        widget.layerComboBox->setVAlign(ftk::VAlign::Center);
                        widget.layerComboBox->setTooltip("Set the current layer.");
                        // Layer names can be long -- and are, in a multi part
                        // EXR. The menu still shows them whole.
                        // Kept from the end: layer names share a prefix and
                        // differ where they finish.
                        widget.layerComboBox->setElide(12, ftk::ElideMode::Left);
                        // A file with one layer has nothing to choose.
                        widget.layerComboBox->setVisible(
                            item->videoLayers.size() > 1);

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
                                controlsLayout);
                            widget.rangeButton->setVAlign(ftk::VAlign::Center);
                            widget.rangeButton->setTooltip(
                                "The frame range of the sequence.");

                            auto buttonWeak =
                                std::weak_ptr<ftk::ToolButton>(widget.rangeButton);
                            widget.rangeButton->setClickedCallback(
                                [this, item, range, buttonWeak]
                                {
                                    _showRangePopup(item, range, buttonWeak.lock());
                                });
                        }

                        // Last, so the one control every row has lines
                        // up at the right edge whatever else the row
                        // holds.
                        widget.bButton = ftk::ToolButton::create(context, "B", controlsLayout);
                        // Its own color, not the row's: a checked "B" on the
                        // checked "A" row would vanish into it.
                        widget.bButton->setCheckedRole(ftk::ColorRole::Blue);
                        const auto i = std::find(b.begin(), b.end(), item);
                        widget.bButton->setChecked(i != b.end());
                        ftk::setScreenshotTag(
                            widget.bButton,
                            i != b.end() ? "Files.BFile" : "");
                        widget.bButton->setVAlign(ftk::VAlign::Center);
                        widget.bButton->setTooltip("Set the B file(s).");
                        p.bButtonGroup->addButton(widget.bButton);

                        // Breathing room after the "B": the row's edge is
                        // the item's edge.
                        auto spacer = ftk::Spacer::create(
                            context, ftk::Orientation::Horizontal, controlsLayout);
                        spacer->setSpacingRole(ftk::SizeRole::SpacingSmall);

                        widget.button->setWidget(rowLayout);

                                                p.widgets.push_back(widget);

                        if (0 == row)
                        {
                            ftk::setScreenshotTag(
                                widget.layerComboBox,
                                "Files.CurrentLayer");
                            ftk::setScreenshotTag(
                                widget.rangeButton,
                                "Files.FrameRange");
                        }
                        ++row;
                    }
                    if (value.empty())
                    {
                        auto label = ftk::Label::create(context, "No files open", p.fileList);
                        label->setMarginRole(ftk::SizeRole::Margin);
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
            if (!button)
                return;
            // The button toggles, the way the slider popups do: the press
            // falls through the open popup to the button, and the click
            // closes it.
            if (p.rangePopup)
            {
                p.rangePopup->close();
                p.rangePopup.reset();
                return;
            }
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
            for (size_t i = 0; i < p.widgets.size(); ++i)
            {
                const auto& widget = p.widgets[i];
                widget.button->setChecked(widget.item == value);
                ftk::setScreenshotTag(
                    widget.button,
                    widget.item == value ? "Files.CurrentFile" : "");
                if (widget.item == value)
                {
                    // The keyboard starts from "A".
                    p.fileList->setCurrent(static_cast<int>(i));
                }
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
            p.differenceGainSlider->setValue(value.differenceGain);
            p.sameSizeCheckBox->setChecked(value.sameSize);

            p.compareLayout->setRowVisible(p.wipeXSlider, value.compare == tl::Compare::Wipe);
            p.compareLayout->setRowVisible(p.wipeYSlider, value.compare == tl::Compare::Wipe);
            p.compareLayout->setRowVisible(p.wipeRotationSlider, value.compare == tl::Compare::Wipe);
            p.compareLayout->setRowVisible(p.overlaySlider, value.compare == tl::Compare::Overlay);
            p.compareLayout->setRowVisible(
                p.differenceGainSlider, value.compare == tl::Compare::Difference);
        }

        void FilesTool::sizeHintEvent(const ftk::SizeHintEvent& event)
        {
            IToolWidget::sizeHintEvent(event);
            FTK_P();
            p.handle = event.style->getSizeRole(
                ftk::SizeRole::Handle, event.displayScale);
        }

        void FilesTool::drawOverlayEvent(
            const ftk::Box2I& drawRect,
            const ftk::DrawEvent& event)
        {
            IToolWidget::drawOverlayEvent(drawRect, event);
            FTK_P();
            if (p.dropTarget != -1)
            {
                const ftk::Box2I g = _getDropGeom(p.dropTarget);
                if (g.isValid())
                {
                    event.render->drawRect(
                        g,
                        event.style->getColorRole(ftk::ColorRole::Checked));
                }
            }
        }

        void FilesTool::dragEnterEvent(ftk::DragDropEvent& event)
        {
            FTK_P();
            if (auto data = std::dynamic_pointer_cast<ui::FileDragDropData>(event.data))
            {
                event.accept = true;
                p.dropTarget = _dropIndex(event.pos, data);
                setDrawUpdate();
            }
        }

        void FilesTool::dragLeaveEvent(ftk::DragDropEvent& event)
        {
            FTK_P();
            if (std::dynamic_pointer_cast<ui::FileDragDropData>(event.data))
            {
                event.accept = true;
                p.dropTarget = -1;
                setDrawUpdate();
            }
        }

        void FilesTool::dragMoveEvent(ftk::DragDropEvent& event)
        {
            FTK_P();
            if (auto data = std::dynamic_pointer_cast<ui::FileDragDropData>(event.data))
            {
                event.accept = true;
                const int dropTarget = _dropIndex(event.pos, data);
                if (dropTarget != p.dropTarget)
                {
                    p.dropTarget = dropTarget;
                    setDrawUpdate();
                }
            }
        }

        void FilesTool::dropEvent(ftk::DragDropEvent& event)
        {
            FTK_P();
            if (auto data = std::dynamic_pointer_cast<ui::FileDragDropData>(event.data))
            {
                event.accept = true;
                if (p.dropTarget != -1)
                {
                    if (auto app = _app.lock())
                    {
                        const auto& files = app->getFilesModel()->getFiles();
                        const auto i = std::find(
                            files.begin(), files.end(), data->getItem());
                        if (i != files.end())
                        {
                            const int from = static_cast<int>(i - files.begin());
                            int to = p.dropTarget;
                            if (from < to)
                            {
                                --to;
                            }
                            app->getFilesModel()->move(from, to);
                        }
                    }
                }
                p.dropTarget = -1;
                setDrawUpdate();
            }
        }

        ftk::Box2I FilesTool::_getRowGeom(size_t index) const
        {
            FTK_P();
            return p.widgets[index].button->getGeometry();
        }

        int FilesTool::_dropIndex(
            const ftk::V2I& pos,
            const std::shared_ptr<ui::FileDragDropData>& data) const
        {
            FTK_P();
            int out = _getDropIndex(pos);
            // The gaps around the dragged row itself are where it already
            // is: dropping there moves nothing, so nothing is shown.
            const auto i = std::find_if(
                p.widgets.begin(),
                p.widgets.end(),
                [&data](const FileWidget& w)
                {
                    return w.item == data->getItem();
                });
            if (i != p.widgets.end())
            {
                const int from = static_cast<int>(i - p.widgets.begin());
                if (out == from || out == from + 1)
                {
                    out = -1;
                }
            }
            return out;
        }

        int FilesTool::_getDropIndex(const ftk::V2I& pos) const
        {
            FTK_P();
            int out = -1;
            // Only over the rows themselves, with a little reach: the tool
            // also holds the comparison section, and a drop there should
            // mean nothing.
            if (!p.widgets.empty() &&
                ftk::contains(ftk::margin(p.fileList->getGeometry(), p.handle), pos))
            {
                out = 0;
                for (size_t i = 0; i < p.widgets.size(); ++i)
                {
                    if (pos.y < ftk::center(_getRowGeom(i)).y)
                    {
                        break;
                    }
                    ++out;
                }
            }
            return out;
        }

        ftk::Box2I FilesTool::_getDropGeom(int index) const
        {
            FTK_P();
            ftk::Box2I out;
            if (!p.widgets.empty())
            {
                const ftk::Box2I& layoutGeom = p.fileList->getGeometry();
                const int size = static_cast<int>(p.widgets.size());
                // Centered in the gap between the rows; at the ends there
                // is no gap, so the row's own edge is the line.
                int y = 0;
                if (0 == index)
                {
                    y = _getRowGeom(0).min.y;
                }
                else if (index < size)
                {
                    y = (_getRowGeom(index - 1).max.y +
                        _getRowGeom(index).min.y) / 2;
                }
                else
                {
                    y = _getRowGeom(size - 1).max.y;
                }
                out = ftk::Box2I(
                    layoutGeom.min.x,
                    y - p.handle / 2,
                    layoutGeom.w(),
                    p.handle);
            }
            return out;
        }
    }
}
