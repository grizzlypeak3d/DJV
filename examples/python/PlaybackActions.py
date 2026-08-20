# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the DJV project.

import opentimelineio as otio
import ftkPy as ftk
import tlRenderPy as tl
import djvPy as djv

import weakref

class Actions:
    """
    This class provides playback actions.
    """
    def __init__(self, context, app):

        self._player = None
        self.actions = {}
        self.actions["Stop"] = ftk.Action(
            "Stop",
            "PlaybackStop",
            ftk.KeyShortcut(ftk.Key.K),
            self._stopCallback)
        self.actions["Stop"].tooltip = "Stop playback."

        self.actions["Forward"] = ftk.Action(
            "Forward",
            "PlaybackForward",
            ftk.KeyShortcut(ftk.Key.L),
            self._forwardCallback)
        self.actions["Forward"].tooltip = "Start forward playback."

        self.actions["Reverse"] = ftk.Action(
            "Reverse",
            "PlaybackReverse",
            ftk.KeyShortcut(ftk.Key.J),
            self._reverseCallback)
        self.actions["Reverse"].tooltip = "Start reverse playback."

        self.actions["TogglePlayback"] = ftk.Action(
            "Toggle Playback",
            ftk.KeyShortcut(ftk.Key.Space),
            self._togglePlaybackCallback)
        self.actions["TogglePlayback"].tooltip = "Toggle playback."

        self.actions["Start"] = ftk.Action(
            "Start Frame",
            "FrameStart",
            ftk.KeyShortcut(ftk.Key.Up),
            self._startCallback)
        self.actions["Start"].tooltip = "Go to the start frame."

        self.actions["Prev"] = ftk.Action(
            "Previous Frame",
            "FramePrev",
            ftk.KeyShortcut(ftk.Key.Left),
            self._prevCallback)
        self.actions["Prev"].tooltip = "Go to the previous frame."

        self.actions["Next"] = ftk.Action(
            "Next Frame",
            "FrameNext",
            ftk.KeyShortcut(ftk.Key.Right),
            self._nextCallback)
        self.actions["Next"].tooltip = "Go to the next frame."

        self.actions["End"] = ftk.Action(
            "End Frame",
            "FrameEnd",
            ftk.KeyShortcut(ftk.Key.Down),
            self._endCallback)
        self.actions["End"].tooltip = "Go to the end frame."

        self.actions["SetInPoint"] = ftk.Action(
            "Set In Point",
            ftk.KeyShortcut(ftk.Key.I),
            self._setInPointCallback)
        self.actions["SetInPoint"].tooltip = "Set the in point to the current frame."

        self.actions["ResetInPoint"] = ftk.Action(
            "Reset In Point",
            ftk.KeyShortcut(ftk.Key.I, ftk.KeyModifier.Shift),
            self._resetInPointCallback)
        self.actions["ResetInPoint"].tooltip = "Reset the in point to the start frame."

        self.actions["SetOutPoint"] = ftk.Action(
            "Set Out Point",
            ftk.KeyShortcut(ftk.Key.O),
            self._setOutPointCallback)
        self.actions["SetOutPoint"].tooltip = "Set the out point to the current frame."

        self.actions["ResetOutPoint"] = ftk.Action(
            "Reset Out Point",
            ftk.KeyShortcut(ftk.Key.O, ftk.KeyModifier.Shift),
            self._resetOutPointCallback)
        self.actions["ResetOutPoint"].tooltip = "Reset the out point to the end frame."

        selfWeak = weakref.ref(self)
        self._playerObserver = tl.PlayerObserver(
            app.observePlayer(),
            lambda player: selfWeak()._playerUpdate(player))

    def _stopCallback(self):
        if self._player:
            self._player.stop()

    def _forwardCallback(self):
        if self._player:
            self._player.forward()

    def _reverseCallback(self):
        if self._player:
            self._player.reverse()

    def _togglePlaybackCallback(self):
        if self._player:
            self._player.togglePlayback()

    def _startCallback(self):
        if self._player:
            self._player.timeAction(tl.TimeAction.Start)

    def _prevCallback(self):
        if self._player:
            self._player.timeAction(tl.TimeAction.FramePrev)

    def _nextCallback(self):
        if self._player:
            self._player.timeAction(tl.TimeAction.FrameNext)

    def _endCallback(self):
        if self._player:
            self._player.timeAction(tl.TimeAction.End)

    def _setInPointCallback(self):
        if self._player:
            self._player.setInPoint()

    def _resetInPointCallback(self):
        if self._player:
            self._player.resetInPoint()

    def _setOutPointCallback(self):
        if self._player:
            self._player.setOutPoint()

    def _resetOutPointCallback(self):
        if self._player:
            self._player.resetOutPoint()

    def _playerUpdate(self, player):

        self._player = player

        if player:
            selfWeak = weakref.ref(self)
            self._playbackObserver = tl.PlaybackObserver(
                player.observePlayback,
                lambda value: selfWeak()._playbackUpdate(value))
        else:
            self._playbackObserver = None

        for name in self.actions:
            self.actions[name].enabled = player != None

    def _playbackUpdate(self, playback):

        self.actions["Stop"].checked = tl.Playback.Stop == playback
        self.actions["Forward"].checked = tl.Playback.Forward == playback
        self.actions["Reverse"].checked = tl.Playback.Reverse == playback
