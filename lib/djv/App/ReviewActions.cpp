// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/ReviewActions.h>

#include <djv/App/App.h>
#include <djv/App/MainWindow.h>
#include <djv/Models/AnnotationsModel.h>
#include <djv/Models/DrawModel.h>
#include <djv/Models/FilesModel.h>
#include <djv/Models/MarkersModel.h>

#include <tlRender/Timeline/Player.h>

namespace djv
{
    namespace app
    {
        struct ReviewActions::Private
        {
            bool hasPlayer = false;
            bool hasMarkers = false;
            bool hasMarkerItems = false;
            //! Whether the in/out points narrow the timeline: adding a range
            //! marker is only meaningful when they do.
            bool narrowed = false;

            std::shared_ptr<ftk::ListObserver<std::shared_ptr<models::FilesModelItem> > > filesObserver;
            std::shared_ptr<ftk::Observer<models::DrawTool> > toolObserver;
            std::shared_ptr<ftk::Observer<bool> > enabledObserver;
            std::shared_ptr<ftk::Observer<bool> > hasUndoObserver;
            std::shared_ptr<ftk::Observer<bool> > hasRedoObserver;
            std::shared_ptr<ftk::ListObserver<int> > markersObserver;
            std::shared_ptr<ftk::ListObserver<models::ReviewMarker> > markerItemsObserver;
            std::shared_ptr<ftk::Observer<std::shared_ptr<tl::Player> > > playerObserver;
            std::shared_ptr<ftk::Observer<OTIO_NS::TimeRange> > inOutObserver;
        };

        void ReviewActions::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<MainWindow>& mainWindow)
        {
            IActions::_init(context, app, "Review");
            FTK_P();

            // Register the commands.
            auto appWeak = std::weak_ptr<App>(app);
            _addCommand(
                "Open",
                "Open a review, replacing the current session.",
                [appWeak](const nlohmann::json&)
                {
                    if (auto app = appWeak.lock())
                    {
                        app->openReviewDialog();
                    }
                });

            _addCommand(
                "Save",
                "Save the current session as a review.",
                [appWeak](const nlohmann::json&)
                {
                    if (auto app = appWeak.lock())
                    {
                        app->saveReview();
                    }
                });

            _addCommand(
                "SaveAs",
                "Save the current session as a new review.",
                [appWeak](const nlohmann::json&)
                {
                    if (auto app = appWeak.lock())
                    {
                        app->saveReviewAs();
                    }
                });

            _addCommand(
                "Import",
                "Import an OTIO timeline as a review: it becomes the "
                "review's first file, and its markers become the feedback.",
                [appWeak](const nlohmann::json&)
                {
                    if (auto app = appWeak.lock())
                    {
                        app->importReviewDialog();
                    }
                });

            _addCommand(
                "Export",
                "Write the review's markers to an OTIO file.",
                [appWeak](const nlohmann::json&)
                {
                    if (auto app = appWeak.lock())
                    {
                        app->exportReviewMarkers();
                    }
                });

            _addCommand(
                "Close",
                "Close the review and reset to the startup state.",
                [appWeak](const nlohmann::json&)
                {
                    if (auto app = appWeak.lock())
                    {
                        app->closeReview();
                    }
                });

            // Selecting a tool turns drawing on, and turning the active tool
            // off gives the left mouse button back to the frame shuttle --
            // the same as the review tool's own buttons.
            _addCheckCommand(
                "Draw",
                "Draw strokes on the frame.",
                [appWeak](const nlohmann::json& args)
                {
                    const bool value = args.at("value").get<bool>();
                    if (auto app = appWeak.lock())
                    {
                        auto drawModel = app->getDrawModel();
                        if (value)
                        {
                            drawModel->setTool(models::DrawTool::Pen);
                            drawModel->setEnabled(true);
                        }
                        else if (models::DrawTool::Pen == drawModel->getTool())
                        {
                            drawModel->setEnabled(false);
                        }
                    }
                });

            // The shapes and text work as the pen does: selecting one turns
            // drawing on with it, and turning the active one off gives the
            // mouse back.
            const std::vector<std::pair<std::string, models::DrawTool> > tools =
            {
                { "Line", models::DrawTool::Line },
                { "Arrow", models::DrawTool::Arrow },
                { "Rectangle", models::DrawTool::Rectangle },
                { "Ellipse", models::DrawTool::Ellipse },
                { "Text", models::DrawTool::Text }
            };
            const std::map<std::string, std::string> toolDocs =
            {
                { "Line", "Draw lines on the frame." },
                { "Arrow", "Draw arrows on the frame." },
                { "Rectangle", "Draw rectangles on the frame." },
                { "Ellipse", "Draw ellipses on the frame." },
                { "Text", "Write text on the frame." }
            };
            for (const auto& i : tools)
            {
                const models::DrawTool tool = i.second;
                _addCheckCommand(
                    i.first,
                    toolDocs.at(i.first),
                    [appWeak, tool](const nlohmann::json& args)
                    {
                        const bool value = args.at("value").get<bool>();
                        if (auto app = appWeak.lock())
                        {
                            auto drawModel = app->getDrawModel();
                            if (value)
                            {
                                drawModel->setTool(tool);
                                drawModel->setEnabled(true);
                            }
                            else if (tool == drawModel->getTool())
                            {
                                drawModel->setEnabled(false);
                            }
                        }
                    });
            }

            _addCheckCommand(
                "OnionSkin",
                "Show the drawings on the frames before and after, faded.",
                [appWeak](const nlohmann::json& args)
                {
                    if (auto app = appWeak.lock())
                    {
                        app->getDrawModel()->setOnionSkin(args.at("value").get<bool>());
                    }
                });

            _addCheckCommand(
                "Erase",
                "Erase the strokes you touch.",
                [appWeak](const nlohmann::json& args)
                {
                    const bool value = args.at("value").get<bool>();
                    if (auto app = appWeak.lock())
                    {
                        auto drawModel = app->getDrawModel();
                        if (value)
                        {
                            drawModel->setTool(models::DrawTool::Eraser);
                            drawModel->setEnabled(true);
                        }
                        else if (models::DrawTool::Eraser == drawModel->getTool())
                        {
                            drawModel->setEnabled(false);
                        }
                    }
                });

            _addCommand(
                "Undo",
                "Undo drawing.",
                [appWeak](const nlohmann::json&)
                {
                    if (auto app = appWeak.lock())
                    {
                        app->getAnnotationsModel()->undo();
                    }
                });

            _addCommand(
                "Redo",
                "Redo drawing.",
                [appWeak](const nlohmann::json&)
                {
                    if (auto app = appWeak.lock())
                    {
                        app->getAnnotationsModel()->redo();
                    }
                });

            _addCommand(
                "ClearDrawing",
                "Remove every stroke on the current frame.",
                [appWeak](const nlohmann::json&)
                {
                    if (auto app = appWeak.lock())
                    {
                        auto player = app->observePlayer()->get();
                        if (player)
                        {
                            // Every active file, so a compared frame's "B"
                            // strokes go with the "A" strokes.
                            std::vector<std::string> ids;
                            for (const auto& i : app->getFilesModel()->getActive())
                            {
                                ids.push_back(i->id);
                            }
                            app->getAnnotationsModel()->clearFrame(
                                ids,
                                player->getCurrentTime());
                        }
                    }
                });

            auto mainWindowWeak = std::weak_ptr<MainWindow>(mainWindow);
            // The identifier stays "AddNote" so saved shortcut bindings
            // survive the marker unification.
            _addCommand(
                "AddNote",
                "Open the review tool and add a marker about the current "
                "frame, edited in place.",
                [mainWindowWeak](const nlohmann::json&)
                {
                    if (auto mainWindow = mainWindowWeak.lock())
                    {
                        mainWindow->addReviewNote();
                    }
                });

            _addCommand(
                "AddRange",
                "Open the review tool and add a marker for the timeline "
                "in/out points, edited in place.",
                [mainWindowWeak](const nlohmann::json&)
                {
                    if (auto mainWindow = mainWindowWeak.lock())
                    {
                        mainWindow->addReviewRange();
                    }
                });

            // Jump between the frames that carry a marker or a drawing. In a
            // review these are the only frames that matter, and stepping to
            // them by hand over a long timeline is the slow part.
            _addCommand(
                "PrevFrame",
                "Go to the previous frame with a marker or a drawing.",
                [appWeak](const nlohmann::json&)
                {
                    if (auto app = appWeak.lock())
                    {
                        app->seekReviewMarker(false);
                    }
                });

            _addCommand(
                "NextFrame",
                "Go to the next frame with a marker or a drawing.",
                [appWeak](const nlohmann::json&)
                {
                    if (auto app = appWeak.lock())
                    {
                        app->seekReviewMarker(true);
                    }
                });

            // Create the actions.
            _actions["Open"] = ftk::Action::create(
                "Open",
                _command("Open"));
            _actions["Save"] = ftk::Action::create(
                "Save",
                _command("Save"));
            _actions["SaveAs"] = ftk::Action::create(
                "Save As...",
                _command("SaveAs"));
            _actions["Import"] = ftk::Action::create(
                "Import...",
                _command("Import"));
            _actions["Export"] = ftk::Action::create(
                "Export...",
                _command("Export"));
            _actions["Close"] = ftk::Action::create(
                "Close",
                _command("Close"));
            _actions["Draw"] = ftk::Action::create(
                "Draw",
                "DrawTool",
                _checkCommand("Draw"));
            _actions["Line"] = ftk::Action::create(
                "Line",
                "DrawLine",
                _checkCommand("Line"));
            _actions["Arrow"] = ftk::Action::create(
                "Arrow",
                "DrawArrow",
                _checkCommand("Arrow"));
            _actions["Rectangle"] = ftk::Action::create(
                "Rectangle",
                "DrawRectangle",
                _checkCommand("Rectangle"));
            _actions["Ellipse"] = ftk::Action::create(
                "Ellipse",
                "DrawEllipse",
                _checkCommand("Ellipse"));
            _actions["Text"] = ftk::Action::create(
                "Text",
                "DrawText",
                _checkCommand("Text"));
            _actions["Erase"] = ftk::Action::create(
                "Erase",
                "Eraser",
                _checkCommand("Erase"));
            _actions["OnionSkin"] = ftk::Action::create(
                "Onion Skin",
                _checkCommand("OnionSkin"));
            _actions["Undo"] = ftk::Action::create(
                "Undo Drawing",
                "Undo",
                _command("Undo"));
            _actions["Redo"] = ftk::Action::create(
                "Redo Drawing",
                "Redo",
                _command("Redo"));
            _actions["ClearDrawing"] = ftk::Action::create(
                "Clear Drawing",
                "Remove",
                _command("ClearDrawing"));
            _actions["AddNote"] = ftk::Action::create(
                "Add Marker",
                _command("AddNote"));
            _actions["AddRange"] = ftk::Action::create(
                "Add Range",
                _command("AddRange"));
            _actions["PrevFrame"] = ftk::Action::create(
                "Previous Marker",
                "ReviewPrev",
                _command("PrevFrame"));
            _actions["NextFrame"] = ftk::Action::create(
                "Next Marker",
                "ReviewNext",
                _command("NextFrame"));

            // The tooltips the review tool's buttons show, richer than the
            // command documentation that fills these in by default: the
            // buttons bind to them, and the shortcut suffix follows the
            // bindings.
            _tooltips["Draw"] =
                "Draw strokes.\n"
                "\n"
                "Click again to stop drawing.";
            _tooltips["Line"] =
                "Draw lines.\n"
                "\n"
                "Hold Shift for a line at a multiple of forty-five degrees. "
                "Click again to stop drawing.";
            _tooltips["Arrow"] =
                "Draw arrows, the head where the drag ends.\n"
                "\n"
                "Hold Shift for an arrow at a multiple of forty-five degrees. "
                "Click again to stop drawing.";
            _tooltips["Rectangle"] =
                "Draw rectangles.\n"
                "\n"
                "Hold Shift for a square. Click again to stop drawing.";
            _tooltips["Ellipse"] =
                "Draw ellipses.\n"
                "\n"
                "Hold Shift for a circle. Click again to stop drawing.";
            _tooltips["Text"] =
                "Write text: click where it starts and type.\n"
                "\n"
                "Return keeps it, Escape lets it go. Click again to stop.";
            _tooltips["Erase"] =
                "Erase the strokes you touch.\n"
                "\n"
                "Click again to stop.";
            _tooltips["OnionSkin"] =
                "Show the drawings on the frames before and after, faded, "
                "behind this frame's.";
            _tooltips["ClearDrawing"] = "Remove every stroke on this frame.";
            _tooltips["AddNote"] =
                "Add a marker about the current frame, written in place.";
            _tooltips["AddRange"] =
                "Add a marker for the timeline in/out points, written in "
                "place.";

            // Register the shortcuts.
            // Alt rather than Shift on the command modifier: Shift+Ctrl+O is
            // "Open with audio", and Shift+Ctrl+S is free but keeping the pair
            // symmetrical is worth more than reusing it.
            _addShortcut(
                "Open",
                "Open review",
                ftk::KeyShortcut(
                    ftk::Key::O,
                    static_cast<int>(ftk::KeyModifier::Alt) |
                    static_cast<int>(ftk::commandKeyModifier)));
            _addShortcut(
                "Save",
                "Save review",
                ftk::KeyShortcut(
                    ftk::Key::S,
                    static_cast<int>(ftk::KeyModifier::Alt) |
                    static_cast<int>(ftk::commandKeyModifier)));
            _addShortcut("SaveAs", "Save review as");
            _addShortcut("Import", "Import a timeline");
            _addShortcut("Export", "Export markers");
            _addShortcut("Close", "Close review");
            // No default keys yet: which keys serve drawing best is still
            // being worked out with the users (#838). The actions are in the
            // shortcuts editor, so any key can be bound today.
            _addShortcut("Draw", "Draw strokes");
            _addShortcut("Line", "Draw lines");
            _addShortcut("Arrow", "Draw arrows");
            _addShortcut("Rectangle", "Draw rectangles");
            _addShortcut("Ellipse", "Draw ellipses");
            _addShortcut("Text", "Write text");
            _addShortcut("Erase", "Erase strokes");
            _addShortcut("OnionSkin", "Onion skin");
            _addShortcut(
                "Undo",
                "Undo drawing",
                ftk::KeyShortcut(ftk::Key::Z, static_cast<int>(ftk::commandKeyModifier)));
            // Ctrl+Y as well where that is the convention; macOS has only the
            // one.
#if defined(__APPLE__)
            const ftk::KeyShortcut redoSecondary;
#else // __APPLE__
            const ftk::KeyShortcut redoSecondary(
                ftk::Key::Y,
                static_cast<int>(ftk::KeyModifier::Control));
#endif // __APPLE__
            _addShortcut(
                "Redo",
                "Redo drawing",
                ftk::KeyShortcut(
                    ftk::Key::Z,
                    static_cast<int>(ftk::KeyModifier::Shift) |
                    static_cast<int>(ftk::commandKeyModifier)),
                redoSecondary);
            _addShortcut("ClearDrawing", "Clear drawing");
            _addShortcut("AddNote", "Add a marker");
            _addShortcut("AddRange", "Add a range");
            // Shift and Control on the arrows are already taken by the X10 and
            // X100 frame steps.
            _addShortcut(
                "PrevFrame",
                "Previous marker",
                ftk::KeyShortcut(
                    ftk::Key::Left,
                    static_cast<int>(ftk::KeyModifier::Alt)));
            _addShortcut(
                "NextFrame",
                "Next marker",
                ftk::KeyShortcut(
                    ftk::Key::Right,
                    static_cast<int>(ftk::KeyModifier::Alt)));

            _shortcutsUpdate(app->getSettingsModel()->getShortcuts());

            p.filesObserver = ftk::ListObserver<std::shared_ptr<models::FilesModelItem> >::create(
                app->getFilesModel()->observeFiles(),
                [this](const std::vector<std::shared_ptr<models::FilesModelItem> >& value)
                {
                    // There is nothing to save, and nothing to close, until a
                    // file is open. Opening a review stays available.
                    _actions["Save"]->setEnabled(!value.empty());
                    _actions["SaveAs"]->setEnabled(!value.empty());
                    _actions["Close"]->setEnabled(!value.empty());
                });

            auto drawModel = app->getDrawModel();
            p.toolObserver = ftk::Observer<models::DrawTool>::create(
                drawModel->observeTool(),
                [this, appWeak](models::DrawTool)
                {
                    if (auto app = appWeak.lock())
                    {
                        _drawStateUpdate(app);
                    }
                });
            p.enabledObserver = ftk::Observer<bool>::create(
                drawModel->observeEnabled(),
                [this, appWeak](bool)
                {
                    if (auto app = appWeak.lock())
                    {
                        _drawStateUpdate(app);
                    }
                });

            p.hasUndoObserver = ftk::Observer<bool>::create(
                app->getAnnotationsModel()->observeHasUndo(),
                [this](bool value)
                {
                    _actions["Undo"]->setEnabled(value);
                });
            p.hasRedoObserver = ftk::Observer<bool>::create(
                app->getAnnotationsModel()->observeHasRedo(),
                [this](bool value)
                {
                    _actions["Redo"]->setEnabled(value);
                });

            p.markerItemsObserver = ftk::ListObserver<models::ReviewMarker>::create(
                app->getMarkersModel()->observeMarkers(),
                [this](const std::vector<models::ReviewMarker>& value)
                {
                    FTK_P();
                    p.hasMarkerItems = !value.empty();
                    _actions["Export"]->setEnabled(
                        p.hasPlayer && p.hasMarkerItems);
                });

            p.markersObserver = ftk::ListObserver<int>::create(
                app->observeReviewMarkers(),
                [this](const std::vector<int>& value)
                {
                    _p->hasMarkers = !value.empty();
                    _markersUpdate();
                });

            p.playerObserver = ftk::Observer<std::shared_ptr<tl::Player> >::create(
                app->observePlayer(),
                [this](const std::shared_ptr<tl::Player>& value)
                {
                    FTK_P();
                    p.hasPlayer = value.get();
                    _actions["Draw"]->setEnabled(p.hasPlayer);
                    _actions["Erase"]->setEnabled(p.hasPlayer);
                    _actions["ClearDrawing"]->setEnabled(p.hasPlayer);
                    _actions["AddNote"]->setEnabled(p.hasPlayer);
                    _actions["Export"]->setEnabled(
                        p.hasPlayer && p.hasMarkerItems);
                    if (value)
                    {
                        p.inOutObserver = ftk::Observer<OTIO_NS::TimeRange>::create(
                            value->observeInOutRange(),
                            [this, value](const OTIO_NS::TimeRange& range)
                            {
                                FTK_P();
                                p.narrowed = !tl::compareExact(
                                    range, value->getTimeRange());
                                _actions["AddRange"]->setEnabled(p.narrowed);
                            });
                    }
                    else
                    {
                        p.inOutObserver.reset();
                        p.narrowed = false;
                        _actions["AddRange"]->setEnabled(false);
                    }
                    _markersUpdate();
                });
        }

        ReviewActions::ReviewActions() :
            _p(new Private)
        {}

        ReviewActions::~ReviewActions()
        {}

        std::shared_ptr<ReviewActions> ReviewActions::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<MainWindow>& mainWindow)
        {
            auto out = std::shared_ptr<ReviewActions>(new ReviewActions);
            out->_init(context, app, mainWindow);
            return out;
        }

        void ReviewActions::_drawStateUpdate(const std::shared_ptr<App>& app)
        {
            auto drawModel = app->getDrawModel();
            const bool enabled = drawModel->isEnabled();
            const models::DrawTool tool = drawModel->getTool();
            _actions["Draw"]->setChecked(
                enabled && models::DrawTool::Pen == tool);
            _actions["Erase"]->setChecked(
                enabled && models::DrawTool::Eraser == tool);
        }

        void ReviewActions::_markersUpdate()
        {
            FTK_P();
            // There is nowhere to jump until a frame carries a marker or a
            // drawing.
            const bool enabled = p.hasPlayer && p.hasMarkers;
            _actions["PrevFrame"]->setEnabled(enabled);
            _actions["NextFrame"]->setEnabled(enabled);
        }
    }
}
