# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the DJV project.

import opentimelineio as otio
import ftkPy as ftk
import tlRenderPy as tl
import djvPy as djv

import IActions
import Util

import weakref

class Actions(IActions.IActions):
    """
    This class provides file actions.
    """
    def __init__(self, context, app, mainWindow):
        IActions.IActions.__init__(self, context, app, "File")

        appWeak = weakref.ref(app)
        mainWindowWeak = weakref.ref(mainWindow)

        # Register the commands.
        self._addCommand(
            "Open",
            "Open a file.",
            lambda args: context.getSystemByName("ftk::FileBrowserSystem").open(
                mainWindowWeak(),
                appWeak().open))

        self._addCommand(
            "OpenAudio",
            "Open a file with a separate audio file.",
            lambda args: appWeak().openSeparateAudioDialog())

        self._addCommand(
            "OpenPlaylist",
            "Open a playlist into the file list.",
            lambda args: appWeak().openPlaylist(ftk.Path(args["fileName"]))
                if args and "fileName" in args
                else appWeak().openPlaylistDialog())

        self._addCommand(
            "SavePlaylist",
            "Save the file list as a playlist.",
            lambda args: appWeak().savePlaylist(ftk.Path(args["fileName"]))
                if args and "fileName" in args
                else appWeak().savePlaylistDialog())

        self._addCommand(
            "Close",
            "Close the current file.",
            lambda args: appWeak().getFilesModel().close())

        self._addCommand(
            "CloseAll",
            "Close all files.",
            lambda args: appWeak().getFilesModel().closeAll())

        self._addCommand(
            "Reload",
            "Reload the current file.",
            lambda args: appWeak().reload())

        self._addCommand(
            "Next",
            "Change to the next file.",
            lambda args: appWeak().getFilesModel().next())

        self._addCommand(
            "Prev",
            "Change to the previous file.",
            lambda args: appWeak().getFilesModel().prev())

        self._addCommand(
            "NextMediaReference",
            "Change to the next media reference.",
            lambda args, f = Util.weak(self._nextMediaReference): f())

        self._addCommand(
            "NextLayer",
            "Change to the next layer.",
            lambda args: appWeak().getFilesModel().nextLayer())

        self._addCommand(
            "PrevLayer",
            "Change to the previous layer.",
            lambda args: appWeak().getFilesModel().prevLayer())

        self._addCommand(
            "OpenReview",
            "Open a review, replacing the current session.",
            lambda args: appWeak().openReview(args["fileName"])
                if args and "fileName" in args
                else appWeak().openReviewDialog())

        self._addCommand(
            "SaveReview",
            "Save the current session as a review.",
            lambda args: appWeak().saveReview())

        self._addCommand(
            "SaveReviewAs",
            "Save the current session as a new review.",
            lambda args: appWeak().saveReviewAs())

        self._addCommand(
            "CloseReview",
            "Close the review and reset to the startup state.",
            lambda args: appWeak().closeReview())

        self._addCommand(
            "Exit",
            "Exit the application.",
            lambda args: appWeak().exit())

        # Create the actions.
        self.actions["Open"] = ftk.Action(
            "Open",
            "FileOpen",
            self._command("Open"))
        self.actions["OpenAudio"] = ftk.Action(
            "Open With Audio",
            "FileOpenAudio",
            self._command("OpenAudio"))
        self.actions["OpenPlaylist"] = ftk.Action(
            "Open Playlist",
            self._command("OpenPlaylist"))
        self.actions["OpenPlaylist"].tooltip = (
            "Open a playlist into the file list. Opening a \".otio\" "
            "file normally plays it as a timeline.")
        self.actions["SavePlaylist"] = ftk.Action(
            "Save Playlist",
            self._command("SavePlaylist"))
        self.actions["SavePlaylist"].tooltip = (
            "Save the file list as a \".otio\" playlist.")
        self.actions["OpenReview"] = ftk.Action(
            "Open Review",
            self._command("OpenReview"))
        self.actions["SaveReview"] = ftk.Action(
            "Save Review",
            self._command("SaveReview"))
        self.actions["SaveReviewAs"] = ftk.Action(
            "Save Review As...",
            self._command("SaveReviewAs"))
        self.actions["CloseReview"] = ftk.Action(
            "Close Review",
            self._command("CloseReview"))
        self.actions["Close"] = ftk.Action(
            "Close",
            "FileClose",
            self._command("Close"))
        self.actions["CloseAll"] = ftk.Action(
            "Close All",
            "FileCloseAll",
            self._command("CloseAll"))
        self.actions["Reload"] = ftk.Action(
            "Reload",
            "FileReload",
            self._command("Reload"))
        self.actions["Next"] = ftk.Action(
            "Next",
            "Next",
            self._command("Next"))
        self.actions["Prev"] = ftk.Action(
            "Previous",
            "Prev",
            self._command("Prev"))
        self.actions["NextMediaReference"] = ftk.Action(
            "Next Media Reference",
            "Next",
            self._command("NextMediaReference"))
        self.actions["NextLayer"] = ftk.Action(
            "Next Layer",
            "Next",
            self._command("NextLayer"))
        self.actions["PrevLayer"] = ftk.Action(
            "Previous Layer",
            "Prev",
            self._command("PrevLayer"))
        self.actions["Exit"] = ftk.Action(
            "Exit",
            self._command("Exit"))

        # Register the shortcuts.
        self._addShortcut("Open", ftk.KeyShortcut(ftk.Key.O, ftk.commandKeyModifier))
        self._addShortcut("OpenAudio", ftk.KeyShortcut(
            ftk.Key.O, ftk.KeyModifier.Shift, ftk.commandKeyModifier))
        # Alt rather than Shift on the command modifier: Shift+Ctrl+O is
        # "Open with audio", and Shift+Ctrl+S is free but keeping the
        # pair symmetrical is worth more than reusing it.
        self._addShortcut("OpenReview", "Open review", ftk.KeyShortcut(
            ftk.Key.O, ftk.KeyModifier.Alt, ftk.commandKeyModifier))
        self._addShortcut("SaveReview", "Save review", ftk.KeyShortcut(
            ftk.Key.S, ftk.KeyModifier.Alt, ftk.commandKeyModifier))
        self._addShortcut("SaveReviewAs", "Save review as")
        self._addShortcut("CloseReview", "Close review")
        self._addShortcut("Close", ftk.KeyShortcut(ftk.Key.E, ftk.commandKeyModifier))
        self._addShortcut("CloseAll", ftk.KeyShortcut(
            ftk.Key.E, ftk.KeyModifier.Shift, ftk.commandKeyModifier))
        self._addShortcut("Reload", ftk.KeyShortcut(
            ftk.Key.R, ftk.KeyModifier.Shift, ftk.commandKeyModifier))
        self._addShortcut("Next", ftk.KeyShortcut(ftk.Key.PageDown, ftk.KeyModifier.Control))
        self._addShortcut("Prev", ftk.KeyShortcut(ftk.Key.PageUp, ftk.KeyModifier.Control))
        self._addShortcut("NextMediaReference", ftk.KeyShortcut(ftk.Key.M, ftk.KeyModifier.Shift))
        self._addShortcut("NextLayer", ftk.KeyShortcut(ftk.Key.Equals, ftk.KeyModifier.Control))
        self._addShortcut("PrevLayer", ftk.KeyShortcut(ftk.Key.Minus, ftk.KeyModifier.Control))
        self._addShortcut("Exit", ftk.KeyShortcut(ftk.Key.Q, ftk.commandKeyModifier))

        self._shortcutsUpdate(self._settingsModel.shortcuts)

        selfWeak = weakref.ref(self)
        self._filesObserver = djv.models.FilesModelItemListObserver(
            app.getFilesModel().observeFiles,
            lambda files: selfWeak()._filesUpdate(files))
        self._aObserver = djv.models.FilesModelItemObserver(
            app.getFilesModel().observeA,
            lambda item: selfWeak()._aUpdate(item))
        self._appWeak = appWeak
        self._playerObserver = tl.PlayerObserver(
            app.observePlayer(),
            lambda player: selfWeak()._playerUpdate(player))

    def _nextMediaReference(self):
        player = self._appWeak().observePlayer().get()
        if player is None:
            return
        # Cycle through the keys used by the timeline, starting from the
        # one in use. An unset key, which leaves the clips as they were
        # authored, is not part of the cycle.
        keys = player.mediaReferenceKeys
        if keys:
            try:
                next = (keys.index(player.mediaReferenceKey) + 1) % len(keys)
            except ValueError:
                next = 0
            player.mediaReferenceKey = keys[next]

    def _filesUpdate(self, files):
        enabled = len(files) > 0
        self.actions["SavePlaylist"].enabled = enabled
        self.actions["Close"].enabled = enabled
        self.actions["CloseAll"].enabled = enabled
        self.actions["Reload"].enabled = enabled
        # There is nothing to save, and nothing to close, until a file
        # is open. Opening a review stays available.
        self.actions["SaveReview"].enabled = enabled
        self.actions["SaveReviewAs"].enabled = enabled
        self.actions["CloseReview"].enabled = enabled
        self.actions["Next"].enabled = len(files) > 1
        self.actions["Prev"].enabled = len(files) > 1

    def _aUpdate(self, item):
        enabled = len(item.videoLayers) > 1 if item else False
        self.actions["NextLayer"].enabled = enabled
        self.actions["PrevLayer"].enabled = enabled

    def _playerUpdate(self, player):
        # There is nothing to cycle through unless the timeline uses more
        # than one media reference key.
        self.actions["NextMediaReference"].enabled = \
            len(player.mediaReferenceKeys) > 1 if player else False
