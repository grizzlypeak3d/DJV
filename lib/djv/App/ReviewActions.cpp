// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/ReviewActions.h>

#include <djv/App/App.h>
#include <djv/App/MainWindow.h>
#include <djv/Models/AnnotationsModel.h>
#include <djv/Models/DrawModel.h>
#include <djv/Models/FilesModel.h>

namespace djv
{
    namespace app
    {
        struct ReviewActions::Private
        {
            bool hasPlayer = false;
            bool hasMarkers = false;

            std::shared_ptr<ftk::ListObserver<std::shared_ptr<models::FilesModelItem> > > filesObserver;
            std::shared_ptr<ftk::Observer<models::DrawTool> > toolObserver;
            std::shared_ptr<ftk::Observer<bool> > enabledObserver;
            std::shared_ptr<ftk::Observer<bool> > hasUndoObserver;
            std::shared_ptr<ftk::Observer<bool> > hasRedoObserver;
            std::shared_ptr<ftk::ListObserver<int> > markersObserver;
            std::shared_ptr<ftk::Observer<std::shared_ptr<tl::Player> > > playerObserver;
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
                "Draw strokes on the frame; e.g., { \"value\": true }.",
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

            _addCheckCommand(
                "Erase",
                "Erase the strokes you touch; e.g., { \"value\": true }.",
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

            auto mainWindowWeak = std::weak_ptr<MainWindow>(mainWindow);
            _addCommand(
                "AddNote",
                "Open the review tool and start a new note, edited in place.",
                [mainWindowWeak](const nlohmann::json&)
                {
                    if (auto mainWindow = mainWindowWeak.lock())
                    {
                        mainWindow->addReviewNote();
                    }
                });

            // Jump between the frames that carry a note or a drawing. In a
            // review these are the only frames that matter, and stepping to
            // them by hand over a long timeline is the slow part.
            _addCommand(
                "PrevFrame",
                "Go to the previous frame with a note or a drawing.",
                [appWeak](const nlohmann::json&)
                {
                    if (auto app = appWeak.lock())
                    {
                        app->seekReviewMarker(false);
                    }
                });

            _addCommand(
                "NextFrame",
                "Go to the next frame with a note or a drawing.",
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
            _actions["Close"] = ftk::Action::create(
                "Close",
                _command("Close"));
            _actions["Draw"] = ftk::Action::create(
                "Draw",
                "DrawTool",
                _checkCommand("Draw"));
            _actions["Erase"] = ftk::Action::create(
                "Erase",
                "Eraser",
                _checkCommand("Erase"));
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
                "Add Note",
                _command("AddNote"));
            _actions["PrevFrame"] = ftk::Action::create(
                "Previous Frame",
                "ReviewPrev",
                _command("PrevFrame"));
            _actions["NextFrame"] = ftk::Action::create(
                "Next Frame",
                "ReviewNext",
                _command("NextFrame"));

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
            _addShortcut("Close", "Close review");
            // No default keys yet: which keys serve drawing best is still
            // being worked out with the users (#838). The actions are in the
            // shortcuts editor, so any key can be bound today.
            _addShortcut("Draw", "Draw strokes");
            _addShortcut("Erase", "Erase strokes");
            _addShortcut("Undo", "Undo drawing");
            _addShortcut("Redo", "Redo drawing");
            _addShortcut("ClearDrawing", "Clear drawing");
            _addShortcut("AddNote", "Add a note");
            // Shift and Control on the arrows are already taken by the X10 and
            // X100 frame steps.
            _addShortcut(
                "PrevFrame",
                "Previous review frame",
                ftk::KeyShortcut(
                    ftk::Key::Left,
                    static_cast<int>(ftk::KeyModifier::Alt)));
            _addShortcut(
                "NextFrame",
                "Next review frame",
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
            // There is nowhere to jump until a frame carries a note or a
            // drawing.
            const bool enabled = p.hasPlayer && p.hasMarkers;
            _actions["PrevFrame"]->setEnabled(enabled);
            _actions["NextFrame"]->setEnabled(enabled);
        }
    }
}
