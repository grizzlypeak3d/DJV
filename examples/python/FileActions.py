# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the DJV project.

import opentimelineio as otio
import ftkPy as ftk
import tlRenderPy as tl
import djvPy as djv

import Util

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

        self.actions["OpenAudio"] = ftk.Action(
            "Open With Audio",
            "FileOpenAudio",
            ftk.KeyShortcut(
                ftk.Key.O, ftk.KeyModifier.Shift, ftk.commandKeyModifier),
            lambda: appWeak().openSeparateAudioDialog())
        self.actions["OpenAudio"].tooltip = "Open a file with a separate audio file."

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

        self.actions["NextLayer"] = ftk.Action(
            "Next Layer",
            "Next",
            ftk.KeyShortcut(ftk.Key.Equals, ftk.KeyModifier.Control),
            lambda: appWeak().getFilesModel().nextLayer())
        self.actions["NextLayer"].tooltip = "Change to the next layer."

        self.actions["PrevLayer"] = ftk.Action(
            "Previous Layer",
            "Prev",
            ftk.KeyShortcut(ftk.Key.Minus, ftk.KeyModifier.Control),
            lambda: appWeak().getFilesModel().prevLayer())
        self.actions["PrevLayer"].tooltip = "Change to the previous layer."

        self.actions["NextMediaReference"] = ftk.Action(
            "Next Media Reference",
            "Next",
            ftk.KeyShortcut(ftk.Key.M, ftk.KeyModifier.Shift),
            Util.weak(self._nextMediaReference))
        self.actions["NextMediaReference"].tooltip = "Change to the next media reference."

        self.actions["Exit"] = ftk.Action(
            "Exit",
            ftk.KeyShortcut(ftk.Key.Q, ftk.commandKeyModifier),
            lambda: appWeak().exit())

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
        self.actions["Close"].enabled = enabled
        self.actions["CloseAll"].enabled = enabled
        self.actions["Reload"].enabled = enabled
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
