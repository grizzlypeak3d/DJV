// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/ReviewTool.h>

#include <djv/App/App.h>
#include <djv/Models/AnnotationsModel.h>
#include <djv/Models/DrawModel.h>
#include <djv/Models/FilesModel.h>
#include <djv/Models/MarkersModel.h>
#include <djv/Models/TimeUnitsModel.h>

#include <tlRender/Timeline/Player.h>

#include <ftk/UI/Bellows.h>
#include <ftk/UI/ColorDot.h>
#include <ftk/UI/ColorSwatch.h>
#include <ftk/UI/DialogSystem.h>
#include <ftk/UI/Divider.h>
#include <ftk/UI/FloatEditSlider.h>
#include <ftk/UI/IWindow.h>
#include <ftk/UI/ItemButton.h>
#include <ftk/UI/Label.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/ScreenshotTag.h>
#include <ftk/UI/ScrollWidget.h>
#include <ftk/UI/TextEdit.h>
#include <ftk/UI/ToolButton.h>

#include <ftk/Core/Format.h>
#include <ftk/Core/String.h>
#include <ftk/Core/Timer.h>

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
            //! Markers are stored in UTC so that a review stays unambiguous
            //! when it travels between time zones, which means the stored
            //! digits are not the ones to show: they must be converted to the
            //! reader's local time, or a marker made at 19:22 reads 17:22.
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

            //! Whether a marker's span is a single frame.
            bool isSingleFrame(const OTIO_NS::TimeRange& range)
            {
                return range.duration().value() <= 1.0;
            }

            //! Format a range in the current time units, so the labels
            //! read the same as the times on the timeline. A single frame is
            //! one value; a span is both ends, inclusive -- the range reads
            //! as the frames you will see, not as a half-open interval.
            std::string formatRange(
                const OTIO_NS::TimeRange& range,
                tl::TimeUnits units)
            {
                if (isSingleFrame(range))
                {
                    return tl::timeToText(range.start_time(), units);
                }
                // Frame numbers read as one token; the longer forms need
                // the space.
                const char* separator =
                    tl::TimeUnits::Frames == units ? "-" : " - ";
                return
                    tl::timeToText(range.start_time(), units) +
                    separator +
                    tl::timeToText(range.end_time_inclusive(), units);
            }

            //! The title a marker's row shows: the name where there is one,
            //! its frames otherwise, and what it is about failing both.
            std::string markerTitle(
                const models::ReviewMarker& marker,
                tl::TimeUnits units)
            {
                if (!marker.name.empty())
                {
                    return marker.name;
                }
                if (marker.range.has_value())
                {
                    return formatRange(*marker.range, units);
                }
                return "No frame";
            }

            //! Whether the marker speaks about the frame being shown.
            bool markerShowing(
                const models::ReviewMarker& marker,
                const std::optional<OTIO_NS::RationalTime>& currentTime)
            {
                return
                    marker.range.has_value() &&
                    currentTime.has_value() &&
                    marker.range->contains(*currentTime);
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
            std::shared_ptr<ftk::ToolButton> addNoteButton;
            std::shared_ptr<ftk::ToolButton> addRangeButton;
            std::shared_ptr<ftk::ToolButton> deleteButton;
            std::shared_ptr<ftk::VerticalLayout> markerListLayout;
            //! The row buttons, by marker identifier, so the highlight can be
            //! moved without rebuilding the list.
            std::map<std::string, std::shared_ptr<ftk::ItemButton> > markerButtons;
            //! The row color dots, by marker identifier, so a color
            //! change can update in place rather than rebuild.
            std::map<std::string, std::shared_ptr<ftk::ColorDot> > swatches;
            //! A list row: the item, what it stands for, and its frames.
            struct RowRef
            {
                std::shared_ptr<ftk::ItemButton> button;
                std::string id;
                std::optional<OTIO_NS::TimeRange> range;
            };
            //! The rows in display order, for the arrow keys.
            std::vector<RowRef> itemOrder;
            //! The row with the key focus, or empty: what the header delete
            //! button acts on.
            std::string focusedId;
            std::vector<models::ReviewMarker> markers;

            std::shared_ptr<ftk::ColorSwatch> colorSwatch;
            std::shared_ptr<ftk::ToolButton> penButton;
            std::shared_ptr<ftk::ToolButton> eraserButton;
            std::shared_ptr<ftk::FloatEditSlider> sizeSlider;
            std::shared_ptr<ftk::ToolButton> undoButton;
            std::shared_ptr<ftk::ToolButton> redoButton;
            std::shared_ptr<ftk::ToolButton> clearDrawingButton;

            std::map<std::string, std::shared_ptr<ftk::Bellows> > bellows;
            std::shared_ptr<ftk::ScrollWidget> scrollWidget;

            //! The marker being edited in place, or empty.
            std::string editingId;
            //! The live in-place editor and its item, for the commit to read
            //! and the scroll to find.
            std::shared_ptr<ftk::TextEdit> editEdit;
            std::shared_ptr<ftk::ItemButton> editItem;
            //! Scroll the edited item into view on the next layout, when the
            //! rebuilt list has a geometry to scroll to.
            bool scrollToEdit = false;
            //! The editor commits when it loses the key focus, deferred a
            //! tick: the loss is reported from inside the editor, and the
            //! commit rebuilds the list that owns it.
            std::shared_ptr<ftk::Timer> commitTimer;

            std::optional<OTIO_NS::RationalTime> currentTime;
            tl::TimeUnits timeUnits = tl::TimeUnits::Timecode;

            std::shared_ptr<tl::Player> player;
            std::optional<OTIO_NS::TimeRange> inOutRange;

            std::shared_ptr<ftk::ListObserver<models::ReviewMarker> > markersObserver;
            std::shared_ptr<ftk::Observer<tl::TimeUnits> > timeUnitsObserver;
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

            // Markers. The list is the bellows content with no margin, so the
            // items run edge to edge.
            // The add buttons live in the bellows title row, so the list
            // reads like the lists elsewhere: + in the header, - on the rows.
            p.addNoteButton = ftk::ToolButton::create(context);
            p.addNoteButton->setIcon("Add");
            p.addNoteButton->setTooltip(
                "Add a marker about the current frame, written in place.");

            p.addRangeButton = ftk::ToolButton::create(context);
            p.addRangeButton->setIcon("FrameInOut");
            p.addRangeButton->setTooltip(
                "Add a marker for the timeline in/out points, written in "
                "place.");

            // One delete for the list, acting on the focused row, rather
            // than one on every row.
            p.deleteButton = ftk::ToolButton::create(context);
            p.deleteButton->setIcon("Remove");
            p.deleteButton->setTooltip(
                "Delete the selected or active marker.");
            p.deleteButton->setEnabled(false);
            // The delete takes the key focus like any button. Taking it
            // puts down the row focus, and the target falls back to the
            // marker on the current frame -- the row's own Delete key is
            // the path that acts on a focused row exactly.

            auto markerToolLayout = ftk::HorizontalLayout::create(context);
            markerToolLayout->setSpacingRole(ftk::SizeRole::None);
            p.addNoteButton->setParent(markerToolLayout);
            p.addRangeButton->setParent(markerToolLayout);
            p.deleteButton->setParent(markerToolLayout);

            p.markerListLayout = ftk::VerticalLayout::create(context);
            p.markerListLayout->setSpacingRole(ftk::SizeRole::None);

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
            // Centered rather than filling the row: the buttons beside it
            // set the row height, and a filled swatch reads as a rectangle.
            p.colorSwatch->setVAlign(ftk::VAlign::Center);
            p.colorSwatch->setColor(drawModel->getColor());
            p.colorSwatch->setTooltip("The stroke colour.");

            p.penButton = ftk::ToolButton::create(context, toolLayout);
            p.penButton->setIcon("DrawTool");
            // Deliberately not checkable: IButton::click() flips its own checked
            // state *after* running the callback, which would invert whatever
            // the model observer had just set. The model stays the only source
            // of truth and the observer drives the highlight.
            p.penButton->setTooltip("Draw strokes.\n\nClick again to stop drawing.");

            p.eraserButton = ftk::ToolButton::create(context, toolLayout);
            p.eraserButton->setIcon("Eraser");
            p.eraserButton->setTooltip("Erase the strokes you touch.\n\nClick again to stop.");

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

            ftk::setScreenshotTag(p.addNoteButton, "Review.AddNote");
            ftk::setScreenshotTag(p.addRangeButton, "Review.AddRange");
            ftk::setScreenshotTag(p.deleteButton, "Review.Delete");
            ftk::setScreenshotTag(p.penButton, "Review.Pen");
            ftk::setScreenshotTag(p.eraserButton, "Review.Eraser");
            ftk::setScreenshotTag(p.clearDrawingButton, "Review.ClearDrawing");

            auto layout = ftk::VerticalLayout::create(context);
            layout->setSpacingRole(ftk::SizeRole::Border);
            p.bellows["Drawing"] = ftk::Bellows::create(context, "Drawing", layout);
            p.bellows["Drawing"]->setWidget(drawingWidget);
            p.bellows["Drawing"]->setOpen(true);
            p.bellows["Markers"] = ftk::Bellows::create(context, "Markers", layout);
            p.bellows["Markers"]->setWidget(p.markerListLayout);
            p.bellows["Markers"]->setToolWidget(markerToolLayout);
            p.bellows["Markers"]->setOpen(true);

            p.scrollWidget = ftk::ScrollWidget::create(context);
            p.scrollWidget->setBorder(false);
            p.scrollWidget->setWidget(layout);
            // The markers have no natural end, so take what room is left
            // rather than a band of its own while other tools sit at the
            // height they need.
            setVStretch(ftk::Stretch::Expanding);
            _setWidget(p.scrollWidget);

            auto appWeak = std::weak_ptr<App>(app);

            p.commitTimer = ftk::Timer::create(context);

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

            p.addNoteButton->setClickedCallback(
                [this]
                {
                    addNote();
                });

            p.addRangeButton->setClickedCallback(
                [this]
                {
                    addRange();
                });

            p.deleteButton->setClickedCallback(
                [this]
                {
                    _deleteMarker();
                });

            p.markersObserver = ftk::ListObserver<models::ReviewMarker>::create(
                app->getMarkersModel()->observeMarkers(),
                [this](const std::vector<models::ReviewMarker>& value)
                {
                    FTK_P();
                    // A color-only change updates the rows in place:
                    // rebuilding would destroy the very swatch whose popup
                    // the user is still picking from.
                    bool colorOnly =
                        !p.markers.empty() &&
                        value.size() == p.markers.size();
                    for (size_t i = 0; colorOnly && i < value.size(); ++i)
                    {
                        models::ReviewMarker recolored = value[i];
                        recolored.color = p.markers[i].color;
                        colorOnly = recolored == p.markers[i];
                    }
                    p.markers = value;
                    if (colorOnly)
                    {
                        for (const auto& marker : value)
                        {
                            const auto i = p.swatches.find(marker.id);
                            if (i != p.swatches.end())
                            {
                                i->second->setColor(marker.color);
                            }
                        }
                    }
                    else
                    {
                        _markersUpdate();
                    }
                });

            // The labels show times, so they follow the units the timeline
            // shows.
            p.timeUnitsObserver = ftk::Observer<tl::TimeUnits>::create(
                app->getTimeUnitsModel()->observeTimeUnits(),
                [this](tl::TimeUnits value)
                {
                    _p->timeUnits = value;
                    _markersUpdate();
                });

            // The list shows every marker; the playhead moves the highlight.
            p.playerObserver = ftk::Observer<std::shared_ptr<tl::Player> >::create(
                app->observePlayer(),
                [this](const std::shared_ptr<tl::Player>& value)
                {
                    FTK_P();
                    p.player = value;
                    // A marker is anchored to the current frame, so without
                    // media there is nothing to attach it to.
                    p.addNoteButton->setEnabled(value.get());
                    if (value)
                    {
                        p.currentTimeObserver = ftk::Observer<OTIO_NS::RationalTime>::create(
                            value->observeCurrentTime(),
                            [this](const OTIO_NS::RationalTime& value)
                            {
                                _p->currentTime = value;
                                _selectionUpdate();
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
                        _selectionUpdate();
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
        }

        void ReviewTool::addRange()
        {
            FTK_P();
            if (!p.player || !p.inOutRange.has_value())
            {
                return;
            }
            // No dialog and no name: the marker's row is titled by its
            // frames, and words belong in its text -- the editor opens in
            // place, and leaving it empty leaves the bare span. The two add
            // gestures differ only in the span they stamp.
            if (auto app = _app.lock())
            {
                _commitMarker();
                const std::string id = app->getMarkersModel()->add(
                    *p.inOutRange, std::string(), std::string());
                _editMarker(id);
            }
        }

        void ReviewTool::setGeometry(const ftk::Box2I& value)
        {
            IToolWidget::setGeometry(value);
            FTK_P();
            // Deferred from the rebuild: only now does the edited item have a
            // geometry to scroll to.
            if (p.scrollToEdit && p.editItem)
            {
                p.scrollToEdit = false;
                if (auto content = p.scrollWidget->getWidget())
                {
                    const ftk::Box2I& g = p.editItem->getGeometry();
                    p.scrollWidget->scrollTo(ftk::Box2I(
                        g.min - content->getGeometry().min,
                        g.size()));
                }
            }
        }

        void ReviewTool::keyPressEvent(ftk::KeyEvent& event)
        {
            IToolWidget::keyPressEvent(event);
            FTK_P();
            // Return with the command modifier, bubbled up from the marker
            // being edited: keep it. (Escape also commits, by way of the
            // editor releasing the focus.)
            if (!event.accept &&
                p.editEdit &&
                ftk::Key::Return == event.key &&
                static_cast<int>(ftk::commandKeyModifier) == event.modifiers)
            {
                event.accept = true;
                _commitMarker();
            }
            // The arrows walk the list rows, bubbled up from a focused row.
            else if (!event.accept &&
                0 == event.modifiers &&
                (ftk::Key::Up == event.key || ftk::Key::Down == event.key))
            {
                event.accept = _navigate(ftk::Key::Down == event.key, event.pos);
            }
            // Delete the selected row, like the header delete button.
            else if (!event.accept &&
                0 == event.modifiers &&
                (ftk::Key::Delete == event.key || ftk::Key::Backspace == event.key))
            {
                if (!p.focusedId.empty())
                {
                    event.accept = true;
                    _deleteMarker();
                }
            }
        }

        void ReviewTool::keyReleaseEvent(ftk::KeyEvent& event)
        {
            IToolWidget::keyReleaseEvent(event);
        }

        void ReviewTool::addNote()
        {
            FTK_P();
            auto app = _app.lock();
            if (!app)
            {
                return;
            }
            auto player = app->observePlayer()->get();
            if (!player)
            {
                return;
            }
            // Keep whatever was being written before starting the next marker.
            _commitMarker();
            // The marker exists from the moment it is asked for -- a bare
            // frame marker is a flag, the way a bare span is -- and its text
            // is edited in place.
            const OTIO_NS::RationalTime time = player->getCurrentTime();
            const std::string id = app->getMarkersModel()->add(
                OTIO_NS::TimeRange(
                    time,
                    OTIO_NS::RationalTime(1.0, time.rate())),
                std::string(),
                std::string());
            _editMarker(id);
        }

        bool ReviewTool::_navigate(bool down, const ftk::V2I& pos)
        {
            FTK_P();
            auto window = getWindow();
            if (!window)
            {
                return false;
            }
            const auto focus = window->getKeyFocus();
            // While the focus is on a row the arrows stay in the list, even
            // at the ends, rather than leaking to whatever the keys mean
            // elsewhere.
            for (size_t i = 0; i < p.itemOrder.size(); ++i)
            {
                if (p.itemOrder[i].button == focus)
                {
                    const size_t j = down ?
                        std::min(i + 1, p.itemOrder.size() - 1) :
                        (i > 0 ? i - 1 : 0);
                    if (j != i)
                    {
                        _goToRow(j);
                    }
                    return true;
                }
            }

            // No row is focused: the arrows enter the list under the cursor,
            // moving off its selection -- hovering the list and pressing an
            // arrow changes the current item straight away.
            const auto i = p.bellows.find("Markers");
            if (i != p.bellows.end() &&
                i->second->isOpen() &&
                !p.itemOrder.empty() &&
                ftk::contains(p.markerListLayout->getGeometry(), pos))
            {
                size_t j = down ? 0 : p.itemOrder.size() - 1;
                for (size_t k = 0; k < p.itemOrder.size(); ++k)
                {
                    if (p.itemOrder[k].button->isChecked())
                    {
                        j = down ?
                            std::min(k + 1, p.itemOrder.size() - 1) :
                            (k > 0 ? k - 1 : 0);
                        break;
                    }
                }
                _goToRow(j);
                return true;
            }
            return false;
        }

        void ReviewTool::_goToRow(size_t index)
        {
            FTK_P();
            if (index >= p.itemOrder.size())
            {
                return;
            }
            p.itemOrder[index].button->takeKeyFocus();
            // Browsing the feedback is looking at the frames it is about,
            // so the arrows follow.
            if (p.itemOrder[index].range.has_value())
            {
                _goToRange(*p.itemOrder[index].range);
            }
        }

        void ReviewTool::_goToRange(const OTIO_NS::TimeRange& range)
        {
            FTK_P();
            if (!p.player)
            {
                return;
            }
            if (isSingleFrame(range))
            {
                _seekTo(range.start_time());
            }
            else
            {
                // Going to a span narrows the timeline to it: the span is
                // what the marker is about, and the in/out points are how
                // the timeline says "these frames". Going elsewhere is what
                // gives the timeline back.
                p.player->setInOutRange(range);
                p.player->seek(range.start_time());
            }
        }

        void ReviewTool::_rowFocus(const std::string& id, bool value)
        {
            FTK_P();
            if (value)
            {
                p.focusedId = id;
            }
            else if (p.focusedId == id)
            {
                p.focusedId.clear();
            }
            _deleteButtonUpdate();
        }

        std::string ReviewTool::_deleteTarget() const
        {
            FTK_P();
            if (!p.focusedId.empty())
            {
                return p.focusedId;
            }
            // The marker on the current frame: what the highlight shows. With
            // several there the first goes; the button stays enabled for the
            // rest.
            for (const auto& row : p.itemOrder)
            {
                if (row.button->isChecked())
                {
                    return row.id;
                }
            }
            return std::string();
        }

        void ReviewTool::_deleteButtonUpdate()
        {
            FTK_P();
            p.deleteButton->setEnabled(!_deleteTarget().empty());
        }

        void ReviewTool::_deleteMarker()
        {
            FTK_P();
            const std::string id = _deleteTarget();
            if (id.empty())
            {
                return;
            }
            // The index before the removal, to land the focus on the
            // neighbour after: repeated deletes then cull a list without
            // re-selecting.
            size_t index = 0;
            for (size_t i = 0; i < p.itemOrder.size(); ++i)
            {
                if (p.itemOrder[i].id == id)
                {
                    index = i;
                    break;
                }
            }
            const bool focused = !p.focusedId.empty();
            if (auto app = _app.lock())
            {
                app->getMarkersModel()->remove(id);
            }
            if (focused && !p.itemOrder.empty())
            {
                // Focus without following the frames: deleting is not
                // browsing.
                const size_t j = std::min(index, p.itemOrder.size() - 1);
                p.itemOrder[j].button->takeKeyFocus();
            }
        }

        void ReviewTool::_seekTo(const OTIO_NS::RationalTime& time)
        {
            FTK_P();
            if (!p.player)
            {
                return;
            }
            // Going to feedback wins over a narrower in/out range: with the
            // target outside it, the seek would move the clock into a span
            // the player cannot show.
            if (!p.player->getInOutRange().contains(time))
            {
                p.player->resetInPoint();
                p.player->resetOutPoint();
            }
            p.player->seek(time);
        }

        void ReviewTool::_editMarker(const std::string& id)
        {
            FTK_P();
            if (id == p.editingId)
            {
                return;
            }
            _commitMarker();
            p.editingId = id;
            _markersUpdate();
        }

        void ReviewTool::_commitMarker()
        {
            FTK_P();
            if (!p.editEdit)
            {
                return;
            }
            const std::string text = ftk::join(p.editEdit->getText(), '\n');
            const std::string id = p.editingId;
            // Clear the state before touching the model: the model observer
            // rebuilds the list, and must not find a half-finished edit.
            p.editingId.clear();
            p.editEdit.reset();
            p.editItem.reset();
            if (auto app = _app.lock())
            {
                if (!id.empty())
                {
                    // Emptied text is kept as emptied: a bare marker is a
                    // flag, not a mistake.
                    app->getMarkersModel()->update(id, text);
                }
            }
            // An unchanged edit does not move the model, so rebuild by hand;
            // the extra rebuild after a model change is harmless.
            _markersUpdate();
        }

        void ReviewTool::_editFocus(
            const std::shared_ptr<ftk::TextEdit>& editor,
            bool value)
        {
            FTK_P();
            if (!editor || editor != p.editEdit)
            {
                return;
            }
            if (value)
            {
                // Focus came back before the deferred commit fired.
                p.commitTimer->stop();
                return;
            }
            // Commit on the next tick: the loss is reported from inside the
            // editor, and the commit rebuilds the list that owns it.
            auto weak = std::weak_ptr<ReviewTool>(
                std::dynamic_pointer_cast<ReviewTool>(shared_from_this()));
            std::weak_ptr<ftk::TextEdit> editWeak(editor);
            p.commitTimer->start(
                std::chrono::milliseconds(0),
                [weak, editWeak]
                {
                    if (auto widget = weak.lock())
                    {
                        if (auto editor = editWeak.lock())
                        {
                            if (editor == widget->_p->editEdit)
                            {
                                widget->_commitMarker();
                            }
                        }
                    }
                });
        }

        void ReviewTool::_markerClicked(const std::string& id)
        {
            FTK_P();
            if (id == p.editingId)
            {
                return;
            }
            const auto i = std::find_if(
                p.markers.begin(),
                p.markers.end(),
                [&id](const models::ReviewMarker& value) { return value.id == id; });
            if (i == p.markers.end())
            {
                return;
            }
            // The first click goes to the marker's frames -- a span narrows
            // the timeline to itself on the way; a click on the marker
            // already showing -- or on one about no frame in particular --
            // opens it for editing.
            if (!i->range.has_value() || markerShowing(*i, p.currentTime))
            {
                _editMarker(id);
            }
            else if (p.player)
            {
                _goToRange(*i->range);
            }
        }

        void ReviewTool::_markersUpdate()
        {
            FTK_P();
            // Let go of any editor before the clear destroys it, so the focus
            // loss it reports on the way out finds nothing left to commit.
            p.editEdit.reset();
            p.editItem.reset();
            p.markerListLayout->clear();
            p.markerButtons.clear();
            p.swatches.clear();
            p.itemOrder.clear();
            auto context = getContext();
            if (!context)
            {
                return;
            }

            // Every marker, so the panel reads as the review's feedback
            // rather than one frame's -- browsing beats following bread
            // crumbs. The model keeps the list in time order, with the
            // markers about no frame in particular first: they speak about
            // the whole review.
            const std::vector<models::ReviewMarker>& value = p.markers;
            if (value.empty())
            {
                // Without this the section is silently empty, which reads as a
                // bug rather than as "nothing to say yet".
                auto label = ftk::Label::create(
                    context,
                    "No markers yet.",
                    p.markerListLayout);
                label->setMarginRole(ftk::SizeRole::MarginSmall);
                label->setTextRole(ftk::ColorRole::TextDisabled);
                _deleteButtonUpdate();
                return;
            }

            auto appWeak = _app;
            std::weak_ptr<ReviewTool> weak(
                std::dynamic_pointer_cast<ReviewTool>(shared_from_this()));
            bool first = true;
            for (const auto& marker : value)
            {
                if (!first)
                {
                    ftk::Divider::create(
                        context, ftk::Orientation::Vertical, p.markerListLayout);
                }
                first = false;
                // The whole marker is one item button: the marker is what the
                // click selects, not one widget inside it.
                auto button = ftk::ItemButton::create(context, p.markerListLayout);
                const bool hasRange = marker.range.has_value();
                const bool editing =
                    !marker.id.empty() && marker.id == p.editingId;
                if (!editing)
                {
                    button->setTooltip(hasRange ?
                        (isSingleFrame(*marker.range) ?
                            "Go to the marker's frame.\n\nClick the marker "
                            "already showing to edit it." :
                            "Go to the marker's frames, setting the timeline "
                            "in/out points to them.\n\nClick the marker "
                            "already showing to edit it.") :
                        "Edit the marker.");
                }
                const std::string id = marker.id;
                button->setClickedCallback(
                    [weak, id]
                    {
                        if (auto widget = weak.lock())
                        {
                            widget->_markerClicked(id);
                        }
                    });
                if (!marker.id.empty())
                {
                    p.markerButtons[marker.id] = button;
                    p.itemOrder.push_back({ button, marker.id, marker.range });
                    button->setFocusCallback(
                        [weak, id](bool value)
                        {
                            if (auto widget = weak.lock())
                            {
                                widget->_rowFocus(id, value);
                            }
                        });
                }

                // One margin around the whole marker, carried by the card,
                // so the header and the text sit evenly inside the item.
                auto card = ftk::VerticalLayout::create(context);
                card->setMarginRole(ftk::SizeRole::MarginInside);
                card->setSpacingRole(ftk::SizeRole::Spacing);
                button->setWidget(card);

                auto header = ftk::HorizontalLayout::create(context, card);
                header->setSpacingRole(ftk::SizeRole::SpacingSmall);

                // The marker's color, editable in place as a small dot --
                // quieter than a swatch in a list of many. The dot takes
                // the click itself, so choosing a color does not read as
                // clicking the row.
                if (!marker.id.empty())
                {
                    auto swatch = ftk::ColorDot::create(context, header);
                    p.swatches[marker.id] = swatch;
                    swatch->setColor(marker.color);
                    swatch->setEditable(true);
                    // The row is the focus unit in this list; the dot acts
                    // like the header delete button and stays out of the
                    // tab order. After setEditable, which opts in.
                    swatch->setAcceptsKeyFocus(false);
                    swatch->setVAlign(ftk::VAlign::Center);
                    swatch->setTooltip("The marker's color.");
                    swatch->setCallback(
                        [weak, id](const ftk::Color4F& value)
                        {
                            if (auto widget = weak.lock())
                            {
                                if (auto app = widget->_app.lock())
                                {
                                    app->getMarkersModel()->updateColor(id, value);
                                }
                            }
                        });
                }

                auto titleLabel = ftk::Label::create(
                    context, markerTitle(marker, p.timeUnits), header);
                titleLabel->setMarginRole(ftk::SizeRole::LabelPad, ftk::SizeRole::None);
                titleLabel->setVAlign(ftk::VAlign::Center);
                titleLabel->setHStretch(ftk::Stretch::Expanding);

                // The frames sit against the right edge, the way the file
                // browser lays out its columns, so a named row says where it
                // points -- unless the title is the frames already, and
                // saying them twice reads as a mistake.
                if (hasRange && !marker.name.empty())
                {
                    const std::string frames = formatRange(*marker.range, p.timeUnits);
                    if (marker.name != frames)
                    {
                        auto framesLabel = ftk::Label::create(context, frames, header);
                        framesLabel->setMarginRole(ftk::SizeRole::LabelPad, ftk::SizeRole::None);
                        framesLabel->setTextRole(ftk::ColorRole::TextDisabled);
                        framesLabel->setVAlign(ftk::VAlign::Center);
                    }
                }

                if (!marker.created.empty())
                {
                    auto createdLabel = ftk::Label::create(
                        context, formatCreated(marker.created), header);
                    createdLabel->setMarginRole(ftk::SizeRole::LabelPad, ftk::SizeRole::None);
                    createdLabel->setTextRole(ftk::ColorRole::TextDisabled);
                    createdLabel->setVAlign(ftk::VAlign::Center);
                }

                if (editing)
                {
                    // Written and edited in place. The marker keeps itself
                    // when the editor loses the focus, or on Command-Return
                    // where the key bubbles to.
                    p.editEdit = ftk::TextEdit::create(context, card);
                    p.editEdit->setSizeHintRole(ftk::SizeRole::ScrollAreaSmall);
                    p.editEdit->setText(ftk::split(marker.text, '\n'));
                    std::weak_ptr<ftk::TextEdit> editWeak(p.editEdit);
                    p.editEdit->setFocusCallback(
                        [weak, editWeak](bool value)
                        {
                            if (auto widget = weak.lock())
                            {
                                widget->_editFocus(editWeak.lock(), value);
                            }
                        });
                    ftk::setScreenshotTag(p.editEdit, "Review.MarkerEdit");
                    p.editItem = button;
                    p.scrollToEdit = true;
                }
                else if (!marker.text.empty())
                {
                    auto textLabel = ftk::Label::create(context, wrapText(marker.text, 40), card);
                    textLabel->setMarginRole(ftk::SizeRole::LabelPad, ftk::SizeRole::None);
                    textLabel->setAlign(ftk::HAlign::Left, ftk::VAlign::Top);
                }
            }
            if (p.editEdit)
            {
                p.editEdit->takeKeyFocus();
            }
            _selectionUpdate();
        }

        void ReviewTool::_selectionUpdate()
        {
            FTK_P();
            // Highlight the markers about the frame being shown.
            for (const auto& marker : p.markers)
            {
                const auto i = p.markerButtons.find(marker.id);
                if (i != p.markerButtons.end())
                {
                    i->second->setChecked(markerShowing(marker, p.currentTime));
                }
            }
            _deleteButtonUpdate();
        }
    }
}
