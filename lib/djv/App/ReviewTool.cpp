// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/ReviewTool.h>

#include <djv/App/App.h>
#include <djv/Models/AnnotationsModel.h>
#include <djv/Models/DrawModel.h>
#include <djv/Models/FilesModel.h>
#include <djv/Models/NotesModel.h>
#include <djv/Models/RangesModel.h>

#include <tlRender/Timeline/Player.h>

#include <ftk/UI/Bellows.h>
#include <ftk/UI/ColorSwatch.h>
#include <ftk/UI/DialogSystem.h>
#include <ftk/UI/Divider.h>
#include <ftk/UI/FloatEditSlider.h>
#include <ftk/UI/ItemButton.h>
#include <ftk/UI/Label.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/ScreenshotTag.h>
#include <ftk/UI/ScrollWidget.h>
#include <ftk/UI/TextEdit.h>
#include <ftk/UI/ToolButton.h>

#include <ftk/Core/Format.h>
#include <ftk/Core/String.h>

#include <algorithm>
#include <ctime>
#include <stdexcept>

namespace djv
{
    namespace app
    {
        namespace
        {
            //! Format an ISO 8601 timestamp for display, e.g.
            //! "2026-07-26T17:22:06Z" -> "26/07/2026 - 19:22" at UTC+2. Falls
            //! back to the raw string if it is not the expected shape.
            //!
            //! Notes are stored in UTC so that a review stays unambiguous when
            //! it travels between time zones, which means the stored digits are
            //! not the ones to show: they must be converted to the reader's
            //! local time, or a note published at 19:22 reads 17:22.
            std::string formatCreated(const std::string& iso)
            {
                if (iso.size() < 16 || 'T' != iso[10])
                {
                    return iso;
                }
                std::tm tm {};
                try
                {
                    tm.tm_year = std::stoi(iso.substr(0, 4)) - 1900;
                    tm.tm_mon = std::stoi(iso.substr(5, 2)) - 1;
                    tm.tm_mday = std::stoi(iso.substr(8, 2));
                    tm.tm_hour = std::stoi(iso.substr(11, 2));
                    tm.tm_min = std::stoi(iso.substr(14, 2));
                    tm.tm_sec = iso.size() >= 19 ? std::stoi(iso.substr(17, 2)) : 0;
                }
                catch (const std::exception&)
                {
                    return iso;
                }
                // The input is UTC, so interpret it as such rather than with
                // std::mktime, which would apply the local offset a second time.
#if defined(_WIN32)
                const std::time_t t = _mkgmtime(&tm);
#else
                const std::time_t t = timegm(&tm);
#endif
                if (static_cast<std::time_t>(-1) == t)
                {
                    return iso;
                }
                std::tm local {};
#if defined(_WIN32)
                localtime_s(&local, &t);
#else
                localtime_r(&t, &local);
#endif
                char buf[32] = {};
                std::strftime(buf, sizeof(buf), "%d/%m/%Y - %H:%M", &local);
                return std::string(buf);
            }

            //! Format a range as its frame bounds, e.g. "0001-0120". Both ends
            //! are inclusive: the range reads as the frames you will see, not
            //! as a half-open interval.
            std::string formatRange(const OTIO_NS::TimeRange& range)
            {
                return ftk::Format("{0}-{1}").
                    arg(static_cast<int>(range.start_time().value()), 4, '0').
                    arg(static_cast<int>(range.end_time_inclusive().value()), 4, '0');
            }

            //! Soft-wrap text to a column. ftk::Label renders newlines but does
            //! not wrap on its own, so long lines would otherwise overflow.
            std::string wrapText(const std::string& text, size_t columns)
            {
                std::string out;
                size_t lineLength = 0;
                for (const auto& line : ftk::split(text, '\n'))
                {
                    if (!out.empty())
                    {
                        out += "\n";
                        lineLength = 0;
                    }
                    for (const auto& word : ftk::split(line, ' '))
                    {
                        if (lineLength > 0 && lineLength + word.size() + 1 > columns)
                        {
                            out += "\n";
                            lineLength = 0;
                        }
                        else if (lineLength > 0)
                        {
                            out += " ";
                            ++lineLength;
                        }
                        out += word;
                        lineLength += word.size();
                    }
                }
                return out;
            }
        }

        struct ReviewTool::Private
        {
            std::shared_ptr<ftk::ToolButton> addRangeButton;
            std::shared_ptr<ftk::VerticalLayout> rangeListLayout;
            //! The row buttons, by range identifier, so the highlight can be
            //! moved without rebuilding the list.
            std::map<std::string, std::shared_ptr<ftk::ItemButton> > rangeButtons;
            std::vector<models::ReviewRange> ranges;
            //! The selected range, or empty. Only one can be active: selecting
            //! is what drives the timeline in/out points.
            std::string selectedRangeId;

            std::shared_ptr<ftk::ColorSwatch> colorSwatch;
            std::shared_ptr<ftk::ToolButton> penButton;
            std::shared_ptr<ftk::ToolButton> eraserButton;
            std::shared_ptr<ftk::FloatEditSlider> sizeSlider;
            std::shared_ptr<ftk::ToolButton> undoButton;
            std::shared_ptr<ftk::ToolButton> redoButton;
            std::shared_ptr<ftk::ToolButton> clearDrawingButton;

            std::shared_ptr<ftk::TextEdit> noteEdit;
            std::shared_ptr<ftk::ToolButton> publishButton;
            std::shared_ptr<ftk::VerticalLayout> noteListLayout;
            //! The frame buttons, by note identifier, so the current frame's
            //! highlight can move without rebuilding the list.
            std::map<std::string, std::shared_ptr<ftk::ItemButton> > noteButtons;
            std::map<std::string, std::shared_ptr<ftk::Bellows> > bellows;

            //! Every note is listed; the frame currently shown only moves
            //! the highlight.
            std::vector<models::ReviewNote> notes;
            std::optional<OTIO_NS::RationalTime> currentTime;

            std::shared_ptr<tl::Player> player;
            std::optional<OTIO_NS::TimeRange> inOutRange;

            std::shared_ptr<ftk::ListObserver<models::ReviewNote> > notesObserver;
            std::shared_ptr<ftk::ListObserver<models::ReviewRange> > rangesObserver;
            std::shared_ptr<ftk::Observer<std::shared_ptr<tl::Player> > > playerObserver;
            std::shared_ptr<ftk::Observer<OTIO_NS::RationalTime> > currentTimeObserver;
            std::shared_ptr<ftk::Observer<OTIO_NS::TimeRange> > inOutRangeObserver;
            std::shared_ptr<ftk::Observer<models::DrawTool> > toolObserver;
            std::shared_ptr<ftk::Observer<bool> > enabledObserver;
            std::shared_ptr<ftk::Observer<ftk::Color4F> > colorObserver;
            std::shared_ptr<ftk::Observer<float> > sizeObserver;
            std::shared_ptr<ftk::Observer<bool> > hasUndoObserver;
            std::shared_ptr<ftk::Observer<bool> > hasRedoObserver;
        };

        void ReviewTool::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<MainWindow>& mainWindow,
            const std::shared_ptr<IWidget>& parent)
        {
            IToolWidget::_init(
                context,
                app,
                mainWindow,
                "Review",
                "Review",
                "djv::app::ReviewTool",
                parent);
            FTK_P();

            // Review ranges.
            auto rangesWidget = ftk::VerticalLayout::create(context);
            rangesWidget->setMarginRole(ftk::SizeRole::MarginSmall);
            rangesWidget->setSpacingRole(ftk::SizeRole::SpacingSmall);

            // The add buttons live in their bellows title rows, so the lists
            // read like the lists elsewhere: + in the header, - on the rows.
            p.addRangeButton = ftk::ToolButton::create(context);
            p.addRangeButton->setIcon("Add");
            p.addRangeButton->setTooltip(
                "Save the timeline in/out points as a named range.");

            p.rangeListLayout = ftk::VerticalLayout::create(context, rangesWidget);
            p.rangeListLayout->setSpacingRole(ftk::SizeRole::None);

            // Drawing.
            auto drawingWidget = ftk::VerticalLayout::create(context);
            drawingWidget->setMarginRole(ftk::SizeRole::MarginSmall);
            drawingWidget->setSpacingRole(ftk::SizeRole::SpacingSmall);

            auto drawModel = app->getDrawModel();

            auto toolLayout = ftk::HorizontalLayout::create(context, drawingWidget);
            toolLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);

            p.colorSwatch = ftk::ColorSwatch::create(context, toolLayout);
            p.colorSwatch->setEditable(true);
            p.colorSwatch->setSizeRole(ftk::SizeRole::MarginLarge);
            p.colorSwatch->setColor(drawModel->getColor());
            p.colorSwatch->setTooltip("The stroke colour.");

            p.penButton = ftk::ToolButton::create(context, toolLayout);
            p.penButton->setIcon("DrawTool");
            // Deliberately not checkable: IButton::click() flips its own checked
            // state *after* running the callback, which would invert whatever
            // the model observer had just set. The model stays the only source
            // of truth and the observer drives the highlight.
            p.penButton->setTooltip("Draw strokes. Click again to stop drawing.");

            p.eraserButton = ftk::ToolButton::create(context, toolLayout);
            p.eraserButton->setIcon("Eraser");
            p.eraserButton->setTooltip("Erase the strokes you touch. Click again to stop.");

            toolLayout->addSpacer(ftk::SizeRole::None, ftk::Stretch::Expanding);

            p.undoButton = ftk::ToolButton::create(context, toolLayout);
            p.undoButton->setIcon("Undo");
            p.undoButton->setTooltip("Undo drawing.");

            p.redoButton = ftk::ToolButton::create(context, toolLayout);
            p.redoButton->setIcon("Redo");
            p.redoButton->setTooltip("Redo drawing.");

            p.clearDrawingButton = ftk::ToolButton::create(context, "Clear", toolLayout);
            p.clearDrawingButton->setTooltip("Remove every stroke on this frame.");

            auto sizeLayout = ftk::HorizontalLayout::create(context, drawingWidget);
            sizeLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            auto sizeLabel = ftk::Label::create(context, "Size:", sizeLayout);
            sizeLabel->setVAlign(ftk::VAlign::Center);
            p.sizeSlider = ftk::FloatEditSlider::create(context, sizeLayout);
            p.sizeSlider->setRange(1.F, 50.F);
            p.sizeSlider->setValue(drawModel->getSize());
            p.sizeSlider->setTooltip("The stroke width, in source pixels.");

            ftk::setScreenshotTag(p.addRangeButton, "Review.AddRange");
            ftk::setScreenshotTag(p.penButton, "Review.Pen");
            ftk::setScreenshotTag(p.eraserButton, "Review.Eraser");
            ftk::setScreenshotTag(p.clearDrawingButton, "Review.ClearDrawing");

            // Notes.
            auto notesWidget = ftk::VerticalLayout::create(context);
            notesWidget->setMarginRole(ftk::SizeRole::MarginSmall);
            notesWidget->setSpacingRole(ftk::SizeRole::SpacingSmall);

            p.noteEdit = ftk::TextEdit::create(context, notesWidget);
            // A few lines: the list below is the review's feedback index, so
            // the leftover height belongs to it, not to the editor.
            p.noteEdit->setSizeHintRole(ftk::SizeRole::ScrollAreaSmall);
            p.noteEdit->setTooltip("Write a note about the current frame.");

            p.publishButton = ftk::ToolButton::create(context);
            p.publishButton->setIcon("Add");
            p.publishButton->setTooltip(
                "Attach the note to the current frame. Return with the\n"
                "command key also adds, from inside the editor.");

            p.noteListLayout = ftk::VerticalLayout::create(context, notesWidget);
            p.noteListLayout->setSpacingRole(ftk::SizeRole::None);
            ftk::setScreenshotTag(p.noteEdit, "Review.NoteEdit");
            ftk::setScreenshotTag(p.publishButton, "Review.AddNote");

            auto layout = ftk::VerticalLayout::create(context);
            layout->setSpacingRole(ftk::SizeRole::Border);
            p.bellows["Ranges"] = ftk::Bellows::create(context, "Ranges", layout);
            p.bellows["Ranges"]->setWidget(rangesWidget);
            p.bellows["Ranges"]->setToolWidget(p.addRangeButton);
            p.bellows["Ranges"]->setOpen(true);
            p.bellows["Drawing"] = ftk::Bellows::create(context, "Drawing", layout);
            p.bellows["Drawing"]->setWidget(drawingWidget);
            p.bellows["Drawing"]->setOpen(true);
            p.bellows["Notes"] = ftk::Bellows::create(context, "Notes", layout);
            p.bellows["Notes"]->setWidget(notesWidget);
            p.bellows["Notes"]->setToolWidget(p.publishButton);
            p.bellows["Notes"]->setOpen(true);

            auto scrollWidget = ftk::ScrollWidget::create(context);
            scrollWidget->setBorder(false);
            scrollWidget->setWidget(layout);
            // The notes have no natural end, so take what room is left
            // rather than a band of its own while other tools sit at the
            // height they need.
            setVStretch(ftk::Stretch::Expanding);
            _setWidget(scrollWidget);

            auto appWeak = std::weak_ptr<App>(app);

            p.colorSwatch->setCallback(
                [appWeak](const ftk::Color4F& value)
                {
                    if (auto app = appWeak.lock())
                    {
                        app->getDrawModel()->setColor(value);
                    }
                });

            // Pen and eraser are the only way in and out of drawing: selecting
            // one turns drawing on, clicking the active one turns it off and
            // gives the left mouse button back to the frame shuttle.
            auto toolClicked = [appWeak](models::DrawTool tool)
            {
                if (auto app = appWeak.lock())
                {
                    auto drawModel = app->getDrawModel();
                    const bool active =
                        drawModel->isEnabled() && drawModel->getTool() == tool;
                    drawModel->setTool(tool);
                    drawModel->setEnabled(!active);
                }
            };
            p.penButton->setClickedCallback(
                [toolClicked] { toolClicked(models::DrawTool::Pen); });
            p.eraserButton->setClickedCallback(
                [toolClicked] { toolClicked(models::DrawTool::Eraser); });

            p.sizeSlider->setCallback(
                [appWeak](float value)
                {
                    if (auto app = appWeak.lock())
                    {
                        app->getDrawModel()->setSize(value);
                    }
                });

            p.undoButton->setClickedCallback(
                [appWeak]
                {
                    if (auto app = appWeak.lock())
                    {
                        app->getAnnotationsModel()->undo();
                    }
                });
            p.redoButton->setClickedCallback(
                [appWeak]
                {
                    if (auto app = appWeak.lock())
                    {
                        app->getAnnotationsModel()->redo();
                    }
                });

            p.clearDrawingButton->setClickedCallback(
                [appWeak]
                {
                    if (auto app = appWeak.lock())
                    {
                        const auto& a = app->getFilesModel()->getA();
                        auto player = app->observePlayer()->get();
                        if (a && player)
                        {
                            app->getAnnotationsModel()->clearFrame(
                                a->id,
                                player->getCurrentTime());
                        }
                    }
                });

            p.toolObserver = ftk::Observer<models::DrawTool>::create(
                drawModel->observeTool(),
                [this](models::DrawTool)
                {
                    _drawStateUpdate();
                });

            p.enabledObserver = ftk::Observer<bool>::create(
                drawModel->observeEnabled(),
                [this](bool)
                {
                    _drawStateUpdate();
                });

            p.colorObserver = ftk::Observer<ftk::Color4F>::create(
                drawModel->observeColor(),
                [this](const ftk::Color4F& value)
                {
                    _p->colorSwatch->setColor(value);
                });

            p.sizeObserver = ftk::Observer<float>::create(
                drawModel->observeSize(),
                [this](float value)
                {
                    _p->sizeSlider->setValue(value);
                });

            p.hasUndoObserver = ftk::Observer<bool>::create(
                app->getAnnotationsModel()->observeHasUndo(),
                [this](bool value)
                {
                    _p->undoButton->setEnabled(value);
                });

            p.hasRedoObserver = ftk::Observer<bool>::create(
                app->getAnnotationsModel()->observeHasRedo(),
                [this](bool value)
                {
                    _p->redoButton->setEnabled(value);
                });

            p.addRangeButton->setClickedCallback(
                [this]
                {
                    _addRange();
                });

            p.rangesObserver = ftk::ListObserver<models::ReviewRange>::create(
                app->getRangesModel()->observeRanges(),
                [this](const std::vector<models::ReviewRange>& value)
                {
                    _p->ranges = value;
                    _rangesUpdate();
                });

            p.publishButton->setClickedCallback(
                [this]
                {
                    _publish();
                });

            p.notesObserver = ftk::ListObserver<models::ReviewNote>::create(
                app->getNotesModel()->observeNotes(),
                [this](const std::vector<models::ReviewNote>& value)
                {
                    _p->notes = value;
                    _notesUpdate();
                });

            // The list shows every note; the playhead moves the highlight.
            p.playerObserver = ftk::Observer<std::shared_ptr<tl::Player> >::create(
                app->observePlayer(),
                [this](const std::shared_ptr<tl::Player>& value)
                {
                    FTK_P();
                    p.player = value;
                    // A note is anchored to the current frame, so without
                    // media there is nothing to attach it to.
                    p.publishButton->setEnabled(value.get());
                    if (value)
                    {
                        p.currentTimeObserver = ftk::Observer<OTIO_NS::RationalTime>::create(
                            value->observeCurrentTime(),
                            [this](const OTIO_NS::RationalTime& value)
                            {
                                _p->currentTime = value;
                                _noteSelectionUpdate();
                            });

                        p.inOutRangeObserver = ftk::Observer<OTIO_NS::TimeRange>::create(
                            value->observeInOutRange(),
                            [this](const OTIO_NS::TimeRange& value)
                            {
                                _p->inOutRange = value;
                                _inOutUpdate();
                            });
                    }
                    else
                    {
                        p.currentTimeObserver.reset();
                        p.inOutRangeObserver.reset();
                        p.currentTime.reset();
                        p.inOutRange.reset();
                        _noteSelectionUpdate();
                        _inOutUpdate();
                    }
                });

            _loadSettings(p.bellows);
        }

        ReviewTool::ReviewTool() :
            _p(new Private)
        {}

        ReviewTool::~ReviewTool()
        {
            _saveSettings(_p->bellows);
        }

        std::shared_ptr<ReviewTool> ReviewTool::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<MainWindow>& mainWindow,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<ReviewTool>(new ReviewTool);
            out->_init(context, app, mainWindow, parent);
            return out;
        }

        void ReviewTool::_drawStateUpdate()
        {
            FTK_P();
            if (auto app = _app.lock())
            {
                auto drawModel = app->getDrawModel();
                const bool enabled = drawModel->isEnabled();
                const models::DrawTool tool = drawModel->getTool();
                p.penButton->setChecked(enabled && models::DrawTool::Pen == tool);
                p.eraserButton->setChecked(enabled && models::DrawTool::Eraser == tool);
            }
        }

        void ReviewTool::_rangesUpdate()
        {
            FTK_P();
            p.rangeListLayout->clear();
            p.rangeButtons.clear();
            auto context = getContext();
            if (!context)
            {
                return;
            }
            if (p.ranges.empty())
            {
                auto label = ftk::Label::create(
                    context,
                    "No ranges yet.",
                    p.rangeListLayout);
                label->setMarginRole(ftk::SizeRole::MarginSmall);
                label->setTextRole(ftk::ColorRole::TextDisabled);
                return;
            }

            auto appWeak = _app;
            std::weak_ptr<ReviewTool> weak(
                std::dynamic_pointer_cast<ReviewTool>(shared_from_this()));
            // The model keeps the list sorted by start frame.
            for (const auto& range : p.ranges)
            {
                // The whole row is one item button, so the list reads as a
                // list rather than a row of separate widgets.
                auto button = ftk::ItemButton::create(context, p.rangeListLayout);
                // Deliberately not checkable, for the reason given on the pen
                // button: click() would flip the state after the callback and
                // fight the highlight set from the selection.
                button->setTooltip(
                    "Set the timeline in/out points to this range. Click again "
                    "to clear them.");
                const std::string id = range.id;
                button->setClickedCallback(
                    [weak, id]
                    {
                        if (auto widget = weak.lock())
                        {
                            widget->_rangeClicked(id);
                        }
                    });
                p.rangeButtons[id] = button;

                auto row = ftk::HorizontalLayout::create(context);
                row->setMarginRole(ftk::SizeRole::MarginInside);
                row->setSpacingRole(ftk::SizeRole::SpacingSmall);
                button->setWidget(row);

                auto nameLabel = ftk::Label::create(context, range.name, row);
                nameLabel->setMarginRole(ftk::SizeRole::LabelPad, ftk::SizeRole::None);
                nameLabel->setVAlign(ftk::VAlign::Center);
                nameLabel->setHStretch(ftk::Stretch::Expanding);

                // The frames sit against the right edge, the way the file
                // browser lays out its columns, so a named row says where it
                // points -- unless the name is the frames, which the default
                // is, and saying them twice reads as a mistake.
                const std::string frames = range.range.has_value() ?
                    formatRange(*range.range) : std::string();
                if (range.name != frames)
                {
                    auto framesLabel = ftk::Label::create(context, frames, row);
                    framesLabel->setMarginRole(ftk::SizeRole::LabelPad, ftk::SizeRole::None);
                    framesLabel->setTextRole(ftk::ColorRole::TextDisabled);
                    framesLabel->setVAlign(ftk::VAlign::Center);
                }

                auto deleteButton = ftk::ToolButton::create(context, row);
                deleteButton->setIcon("RemoveSmall");
                deleteButton->setTooltip("Delete this range.");
                deleteButton->setClickedCallback(
                    [appWeak, id]
                    {
                        if (auto app = appWeak.lock())
                        {
                            app->getRangesModel()->remove(id);
                        }
                    });
            }
            _rangeSelectionUpdate();
        }

        void ReviewTool::_rangeSelectionUpdate()
        {
            FTK_P();
            for (const auto& i : p.rangeButtons)
            {
                i.second->setChecked(i.first == p.selectedRangeId);
            }
        }

        void ReviewTool::_rangeClicked(const std::string& id)
        {
            FTK_P();
            if (!p.player)
            {
                return;
            }
            if (id == p.selectedRangeId)
            {
                // Clicking the active range clears the in/out points and gives
                // the whole timeline back.
                p.selectedRangeId.clear();
                p.player->resetInPoint();
                p.player->resetOutPoint();
            }
            else
            {
                const auto i = std::find_if(
                    p.ranges.begin(),
                    p.ranges.end(),
                    [&id](const models::ReviewRange& value) { return value.id == id; });
                if (i == p.ranges.end())
                {
                    return;
                }
                // Set the selection first: applying the range makes the in/out
                // observer fire, and it must not read this as a stale highlight.
                p.selectedRangeId = id;
                p.player->setInOutRange(*i->range);
                // Without this the playhead stays outside the range it just set.
                p.player->seek(i->range->start_time());
            }
            _rangeSelectionUpdate();
        }

        void ReviewTool::_inOutUpdate()
        {
            FTK_P();
            // Adding is only meaningful once the in/out points actually narrow
            // the timeline.
            const bool narrowed =
                p.player &&
                p.inOutRange.has_value() &&
                !tl::compareExact(*p.inOutRange, p.player->getTimeRange());
            p.addRangeButton->setEnabled(narrowed);

            // Drop the highlight as soon as the in/out points stop matching the
            // selected range, e.g. after dragging them by hand. Otherwise the
            // next click on that row would read as "clear" when the user meant
            // "apply it again".
            if (!p.selectedRangeId.empty())
            {
                const auto i = std::find_if(
                    p.ranges.begin(),
                    p.ranges.end(),
                    [this](const models::ReviewRange& value)
                    {
                        return value.id == _p->selectedRangeId;
                    });
                if (i == p.ranges.end() || !models::sameRange(i->range, p.inOutRange))
                {
                    p.selectedRangeId.clear();
                    _rangeSelectionUpdate();
                }
            }
        }

        void ReviewTool::_addRange()
        {
            FTK_P();
            auto context = getContext();
            if (!context || !p.player)
            {
                return;
            }
            if (!p.inOutRange.has_value())
            {
                return;
            }
            const OTIO_NS::TimeRange range = *p.inOutRange;
            const std::string defaultName = formatRange(range);
            auto appWeak = _app;
            context->getSystem<ftk::DialogSystem>()->input(
                "Add Review Range",
                ftk::Format("Frames {0}").arg(defaultName),
                defaultName,
                getWindow(),
                [appWeak, range, defaultName](const std::string& value)
                {
                    if (auto app = appWeak.lock())
                    {
                        // An emptied field falls back to the frame range rather
                        // than producing a nameless row.
                        app->getRangesModel()->add(
                            range,
                            value.empty() ? defaultName : value);
                    }
                });
        }

        void ReviewTool::keyPressEvent(ftk::KeyEvent& event)
        {
            IToolWidget::keyPressEvent(event);
            // Return with the command modifier, bubbled up from the note
            // editor: the same as the add button.
            if (!event.accept &&
                ftk::Key::Return == event.key &&
                static_cast<int>(ftk::commandKeyModifier) == event.modifiers)
            {
                event.accept = true;
                _publish();
            }
        }

        void ReviewTool::keyReleaseEvent(ftk::KeyEvent& event)
        {
            IToolWidget::keyReleaseEvent(event);
        }

        void ReviewTool::focusNote()
        {
            FTK_P();
            p.noteEdit->takeKeyFocus();
        }

        void ReviewTool::_publish()
        {
            FTK_P();
            const std::string text = ftk::join(p.noteEdit->getText(), '\n');
            if (text.empty())
            {
                return;
            }
            if (auto app = _app.lock())
            {
                // The note is anchored to the frame shown when it is published.
                std::optional<OTIO_NS::RationalTime> time;
                if (auto player = app->observePlayer()->get())
                {
                    time = player->getCurrentTime();
                }
                app->getNotesModel()->add(time, text);
                p.noteEdit->clearText();
            }
        }

        void ReviewTool::_notesUpdate()
        {
            FTK_P();
            p.noteListLayout->clear();
            p.noteButtons.clear();
            auto context = getContext();
            if (!context)
            {
                return;
            }

            // Every note, so the panel reads as the review's feedback rather
            // than one frame's -- browsing beats following bread crumbs. In
            // frame order, with the notes about no frame in particular first:
            // they speak about the whole review.
            std::vector<models::ReviewNote> value = p.notes;
            std::stable_sort(
                value.begin(),
                value.end(),
                [](const models::ReviewNote& a, const models::ReviewNote& b)
                {
                    if (a.time.has_value() != b.time.has_value())
                    {
                        return !a.time.has_value();
                    }
                    if (!a.time.has_value())
                    {
                        return false;
                    }
                    return a.time->value() < b.time->value();
                });
            if (value.empty())
            {
                // Without this the section is silently empty, which reads as a
                // bug rather than as "nothing to say yet".
                auto label = ftk::Label::create(
                    context,
                    "No notes yet.",
                    p.noteListLayout);
                label->setMarginRole(ftk::SizeRole::MarginSmall);
                label->setTextRole(ftk::ColorRole::TextDisabled);
                return;
            }

            auto appWeak = _app;
            for (const auto& note : value)
            {
                // The whole note is one item button, like a range row: the
                // note is what the click selects, not one widget inside it.
                auto button = ftk::ItemButton::create(context, p.noteListLayout);
                const bool hasTime = note.time.has_value();
                if (hasTime)
                {
                    button->setTooltip("Go to the note's frame.");
                }
                const std::optional<OTIO_NS::RationalTime> time = note.time;
                button->setClickedCallback(
                    [appWeak, time]
                    {
                        if (auto app = appWeak.lock())
                        {
                            if (auto player = app->observePlayer()->get())
                            {
                                if (time.has_value())
                                {
                                    player->seek(*time);
                                }
                            }
                        }
                    });
                p.noteButtons[note.id] = button;

                auto card = ftk::VerticalLayout::create(context);
                card->setSpacingRole(ftk::SizeRole::None);
                button->setWidget(card);

                auto header = ftk::HorizontalLayout::create(context, card);
                header->setMarginRole(ftk::SizeRole::MarginInside);
                header->setSpacingRole(ftk::SizeRole::SpacingSmall);

                auto frameLabel = ftk::Label::create(
                    context,
                    hasTime ?
                        ftk::Format("Frame {0}").arg(static_cast<int>(note.time->value())).operator std::string() :
                        std::string("No frame"),
                    header);
                frameLabel->setMarginRole(ftk::SizeRole::LabelPad, ftk::SizeRole::None);
                frameLabel->setVAlign(ftk::VAlign::Center);
                frameLabel->setHStretch(ftk::Stretch::Expanding);

                auto createdLabel = ftk::Label::create(
                    context, formatCreated(note.created), header);
                createdLabel->setMarginRole(ftk::SizeRole::LabelPad, ftk::SizeRole::None);
                createdLabel->setTextRole(ftk::ColorRole::TextDisabled);
                createdLabel->setVAlign(ftk::VAlign::Center);

                auto deleteButton = ftk::ToolButton::create(context, header);
                deleteButton->setIcon("RemoveSmall");
                deleteButton->setTooltip("Delete this note.");
                const std::string id = note.id;
                deleteButton->setClickedCallback(
                    [appWeak, id]
                    {
                        if (auto app = appWeak.lock())
                        {
                            app->getNotesModel()->remove(id);
                        }
                    });

                auto textLabel = ftk::Label::create(context, wrapText(note.text, 40), card);
                textLabel->setMarginRole(ftk::SizeRole::Margin);
                textLabel->setAlign(ftk::HAlign::Left, ftk::VAlign::Top);
            }
            _noteSelectionUpdate();
        }

        void ReviewTool::_noteSelectionUpdate()
        {
            FTK_P();
            // Highlight the notes on the frame being shown.
            for (const auto& note : p.notes)
            {
                const auto i = p.noteButtons.find(note.id);
                if (i != p.noteButtons.end())
                {
                    i->second->setChecked(
                        models::sameTime(note.time, p.currentTime));
                }
            }
        }
    }
}
