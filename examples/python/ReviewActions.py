# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the DJV project.

import ftkPy as ftk
import tlRenderPy as tl
import djvPy as djv

import IActions
import Util

import weakref

class Actions(IActions.IActions):
    """
    This class provides the review actions.
    """
    def __init__(self, context, app, mainWindow):
        IActions.IActions.__init__(self, context, app, "Review")

        appWeak = weakref.ref(app)
        mainWindowWeak = weakref.ref(mainWindow)
        self._hasPlayer = False
        self._hasMarkers = False

        # Register the commands.
        self._addCommand(
            "Open",
            "Open a review, replacing the current session.",
            lambda args: appWeak().openReview(args["fileName"])
                if args and "fileName" in args
                else appWeak().openReviewDialog())

        self._addCommand(
            "Save",
            "Save the current session as a review.",
            lambda args: appWeak().saveReview())

        self._addCommand(
            "SaveAs",
            "Save the current session as a new review.",
            lambda args: appWeak().saveReviewAs())

        self._addCommand(
            "Close",
            "Close the review and reset to the startup state.",
            lambda args: appWeak().closeReview())

        # Selecting a tool turns drawing on, and turning the active tool
        # off gives the left mouse button back to the frame shuttle --
        # the same as the review tool's own buttons.
        self._addCheckCommand(
            "Draw",
            "Draw strokes on the frame; e.g., { \"value\": true }.",
            lambda value, f = Util.weak(self._drawTool):
                f(djv.models.DrawTool.Pen, value))

        self._addCheckCommand(
            "Erase",
            "Erase the strokes you touch; e.g., { \"value\": true }.",
            lambda value, f = Util.weak(self._drawTool):
                f(djv.models.DrawTool.Eraser, value))

        self._addCommand(
            "Undo",
            "Undo drawing.",
            lambda args: appWeak().getAnnotationsModel().undo())

        self._addCommand(
            "Redo",
            "Redo drawing.",
            lambda args: appWeak().getAnnotationsModel().redo())

        def clearDrawing(args):
            app_ = appWeak()
            if app_ is None:
                return
            a = app_.getFilesModel().a
            player = app_.observePlayer().get()
            if a and player:
                app_.getAnnotationsModel().clearFrame(a.id, player.currentTime)
        self._addCommand(
            "ClearDrawing",
            "Remove every stroke on the current frame.",
            clearDrawing)

        self._addCommand(
            "AddNote",
            "Open the review tool with the keyboard focus on the note "
            "editor.",
            lambda args: mainWindowWeak().focusReviewNote())

        # Jump between the frames that carry a note or a drawing. In a
        # review these are the only frames that matter, and stepping to
        # them by hand over a long timeline is the slow part.
        self._addCommand(
            "PrevFrame",
            "Go to the previous frame with a note or a drawing.",
            lambda args: appWeak().seekReviewMarker(False))

        self._addCommand(
            "NextFrame",
            "Go to the next frame with a note or a drawing.",
            lambda args: appWeak().seekReviewMarker(True))

        # Create the actions.
        self.actions["Open"] = ftk.Action(
            "Open",
            self._command("Open"))
        self.actions["Save"] = ftk.Action(
            "Save",
            self._command("Save"))
        self.actions["SaveAs"] = ftk.Action(
            "Save As...",
            self._command("SaveAs"))
        self.actions["Close"] = ftk.Action(
            "Close",
            self._command("Close"))
        self.actions["Draw"] = ftk.Action(
            "Draw",
            "DrawTool",
            checkedCallback = self._checkCommand("Draw"))
        self.actions["Erase"] = ftk.Action(
            "Erase",
            "Eraser",
            checkedCallback = self._checkCommand("Erase"))
        self.actions["Undo"] = ftk.Action(
            "Undo Drawing",
            "Undo",
            self._command("Undo"))
        self.actions["Redo"] = ftk.Action(
            "Redo Drawing",
            "Redo",
            self._command("Redo"))
        self.actions["ClearDrawing"] = ftk.Action(
            "Clear Drawing",
            "Remove",
            self._command("ClearDrawing"))
        self.actions["AddNote"] = ftk.Action(
            "Add Note",
            self._command("AddNote"))
        self.actions["PrevFrame"] = ftk.Action(
            "Previous Frame",
            "ReviewPrev",
            self._command("PrevFrame"))
        self.actions["NextFrame"] = ftk.Action(
            "Next Frame",
            "ReviewNext",
            self._command("NextFrame"))

        # Register the shortcuts.
        # Alt rather than Shift on the command modifier: Shift+Ctrl+O is
        # "Open with audio", and Shift+Ctrl+S is free but keeping the
        # pair symmetrical is worth more than reusing it.
        self._addShortcut("Open", "Open review", ftk.KeyShortcut(
            ftk.Key.O, ftk.KeyModifier.Alt, ftk.commandKeyModifier))
        self._addShortcut("Save", "Save review", ftk.KeyShortcut(
            ftk.Key.S, ftk.KeyModifier.Alt, ftk.commandKeyModifier))
        self._addShortcut("SaveAs", "Save review as")
        self._addShortcut("Close", "Close review")
        # No default keys yet: which keys serve drawing best is still
        # being worked out with the users (#838). The actions are in the
        # shortcuts editor, so any key can be bound today.
        self._addShortcut("Draw", "Draw strokes")
        self._addShortcut("Erase", "Erase strokes")
        self._addShortcut("Undo", "Undo drawing")
        self._addShortcut("Redo", "Redo drawing")
        self._addShortcut("ClearDrawing", "Clear drawing")
        self._addShortcut("AddNote", "Add a note")
        # Shift and Control on the arrows are already taken by the X10
        # and X100 frame steps.
        self._addShortcut("PrevFrame", "Previous review frame",
            ftk.KeyShortcut(ftk.Key.Left, ftk.KeyModifier.Alt))
        self._addShortcut("NextFrame", "Next review frame",
            ftk.KeyShortcut(ftk.Key.Right, ftk.KeyModifier.Alt))

        self._shortcutsUpdate(self._settingsModel.shortcuts)

        self._appWeak = appWeak
        selfWeak = weakref.ref(self)
        self._filesObserver = djv.models.FilesModelItemListObserver(
            app.getFilesModel().observeFiles,
            lambda files: selfWeak()._filesUpdate(files))
        drawModel = app.getDrawModel()
        self._toolObserver = djv.models.DrawToolObserver(
            drawModel.observeTool,
            lambda value: selfWeak()._drawStateUpdate())
        self._enabledObserver = ftk.BoolObserver(
            drawModel.observeEnabled,
            lambda value: selfWeak()._drawStateUpdate())
        self._hasUndoObserver = ftk.BoolObserver(
            app.getAnnotationsModel().observeHasUndo,
            lambda value: selfWeak() and setattr(
                selfWeak().actions["Undo"], "enabled", value))
        self._hasRedoObserver = ftk.BoolObserver(
            app.getAnnotationsModel().observeHasRedo,
            lambda value: selfWeak() and setattr(
                selfWeak().actions["Redo"], "enabled", value))
        self._markersObserver = ftk.IntListObserver(
            app.observeReviewMarkers(),
            lambda markers: selfWeak()._markersUpdate(markers))
        self._playerObserver = tl.PlayerObserver(
            app.observePlayer(),
            lambda player: selfWeak()._playerUpdate(player))

    def _filesUpdate(self, files):
        # There is nothing to save, and nothing to close, until a file
        # is open. Opening a review stays available.
        enabled = len(files) > 0
        self.actions["Save"].enabled = enabled
        self.actions["SaveAs"].enabled = enabled
        self.actions["Close"].enabled = enabled

    def _drawTool(self, tool, value):
        app = self._appWeak()
        if app is None:
            return
        drawModel = app.getDrawModel()
        if value:
            drawModel.tool = tool
            drawModel.enabled = True
        elif tool == drawModel.tool:
            drawModel.enabled = False

    def _drawStateUpdate(self):
        app = self._appWeak()
        if app is None:
            return
        drawModel = app.getDrawModel()
        enabled = drawModel.enabled
        tool = drawModel.tool
        self.actions["Draw"].checked = \
            enabled and djv.models.DrawTool.Pen == tool
        self.actions["Erase"].checked = \
            enabled and djv.models.DrawTool.Eraser == tool

    def _markersUpdate(self, markers):
        if markers is not None:
            self._hasMarkers = len(markers) > 0
        # There is nowhere to jump until a frame carries a note or a
        # drawing.
        enabled = self._hasPlayer and self._hasMarkers
        self.actions["PrevFrame"].enabled = enabled
        self.actions["NextFrame"].enabled = enabled

    def _playerUpdate(self, player):
        self._hasPlayer = player is not None
        self.actions["Draw"].enabled = self._hasPlayer
        self.actions["Erase"].enabled = self._hasPlayer
        self.actions["ClearDrawing"].enabled = self._hasPlayer
        self.actions["AddNote"].enabled = self._hasPlayer
        self._markersUpdate(None)
