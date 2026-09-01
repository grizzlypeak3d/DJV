# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the DJV project.

import ftkPy as ftk
import tlRenderPy as tl
import djvPy as djv

import IActions

import weakref

class Actions(IActions.IActions):
    """
    This class provides the review actions.
    """
    def __init__(self, context, app, mainWindow):
        IActions.IActions.__init__(self, context, app, "Review")

        appWeak = weakref.ref(app)

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

        self._shortcutsUpdate(self._settingsModel.shortcuts)

        selfWeak = weakref.ref(self)
        self._filesObserver = djv.models.FilesModelItemListObserver(
            app.getFilesModel().observeFiles,
            lambda files: selfWeak()._filesUpdate(files))

    def _filesUpdate(self, files):
        # There is nothing to save, and nothing to close, until a file
        # is open. Opening a review stays available.
        enabled = len(files) > 0
        self.actions["Save"].enabled = enabled
        self.actions["SaveAs"].enabled = enabled
        self.actions["Close"].enabled = enabled
