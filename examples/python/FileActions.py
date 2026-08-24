# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the DJV project.

import opentimelineio as otio
import ftkPy as ftk
import tlRenderPy as tl
import djvPy as djv

import weakref

class Actions:
    """
    This class provides file actions.
    """
    def __init__(self, context, app, mainWindow):

        appWeak = weakref.ref(app)
        mainWindowWeak = weakref.ref(mainWindow)
        self.actions = {}
        self.actions["Open"] = ftk.Action(
            "Open",
            "FileOpen",
            ftk.KeyShortcut(ftk.Key.O, ftk.commandKeyModifier),
            lambda: context.getSystemByName("ftk::FileBrowserSystem").open(
                mainWindowWeak(),
                appWeak().open))
        self.actions["Open"].tooltip = "Open an image sequence, movie, or timeline file."

        self.actions["Close"] = ftk.Action(
            "Close",
            "FileClose",
            ftk.KeyShortcut(ftk.Key.E, ftk.commandKeyModifier),
            lambda: appWeak().getFilesModel().close())
        self.actions["Close"].tooltip = "Close the current file."

        self.actions["CloseAll"] = ftk.Action(
            "Close All",
            "FileCloseAll",
            ftk.KeyShortcut(
                ftk.Key.E, ftk.KeyModifier.Shift, ftk.commandKeyModifier),
            lambda: appWeak().getFilesModel().closeAll())
        self.actions["CloseAll"].tooltip = "Close all of the files."

        self.actions["Next"] = ftk.Action(
            "Next File",
            ftk.KeyShortcut(ftk.Key.PageDown, ftk.KeyModifier.Control),
            lambda: appWeak().getFilesModel().next())
        self.actions["Next"].tooltip = "Change to the next file."

        self.actions["Prev"] = ftk.Action(
            "Previous File",
            ftk.KeyShortcut(ftk.Key.PageUp, ftk.KeyModifier.Control),
            lambda: appWeak().getFilesModel().prev())
        self.actions["Prev"].tooltip = "Change to the previous file."

        self.actions["Reload"] = ftk.Action(
            "Reload",
            "FileReload",
            ftk.KeyShortcut(
                ftk.Key.R, ftk.KeyModifier.Shift, ftk.commandKeyModifier),
            lambda: appWeak().getFilesModel().refresh())
        self.actions["Reload"].tooltip = "Reload the current file."

        self.actions["Exit"] = ftk.Action(
            "Exit",
            ftk.KeyShortcut(ftk.Key.Q, ftk.commandKeyModifier),
            lambda: appWeak().exit())

        selfWeak = weakref.ref(self)
        self._filesObserver = djv.models.FilesModelItemListObserver(
            app.getFilesModel().observeFiles,
            lambda files: selfWeak()._filesUpdate(files))

    def _filesUpdate(self, files):
        enabled = len(files) > 0
        self.actions["Close"].enabled = enabled
        self.actions["CloseAll"].enabled = enabled
        self.actions["Reload"].enabled = enabled
        self.actions["Next"].enabled = len(files) > 1
        self.actions["Prev"].enabled = len(files) > 1
