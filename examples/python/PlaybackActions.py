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

        for name, text, key, mod, action, tooltip in [
            ("JumpBack1s", "Jump Back 1s", ftk.Key.J, ftk.KeyModifier.Shift,
             tl.TimeAction.JumpBack1s, "Jump back 1 second."),
            ("JumpBack10s", "Jump Back 10s", ftk.Key.J, ftk.KeyModifier.Control,
             tl.TimeAction.JumpBack10s, "Jump back 10 seconds."),
            ("JumpForward1s", "Jump Forward 1s", ftk.Key.L, ftk.KeyModifier.Shift,
             tl.TimeAction.JumpForward1s, "Jump forward 1 second."),
            ("JumpForward10s", "Jump Forward 10s", ftk.Key.L, ftk.KeyModifier.Control,
             tl.TimeAction.JumpForward10s, "Jump forward 10 seconds."),
        ]:
            a = ftk.Action(
                text,
                ftk.KeyShortcut(key, mod),
                lambda captured = action: self._timeAction(captured))
            a.tooltip = tooltip
            self.actions[name] = a

        # The loop modes are one radio group.
        self.loopGroup = ftk.ActionGroup(ftk.ActionGroupType.Radio)
        for mode in tl.getLoopEnums():
            a = ftk.Action(
                tl.getLabel(mode),
                checkedCallback = lambda checked, captured = mode:
                    self._loopCallback(captured) if checked else None)
            self.actions[tl.to_string(mode)] = a
            self.loopGroup.addAction(a)

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

    def _timeAction(self, value):
        if self._player:
            self._player.timeAction(value)

    def _loopCallback(self, value):
        if self._player:
            self._player.loop = value

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
            self._loopObserver = tl.LoopObserver(
                player.observeLoop,
                lambda value: selfWeak()._loopUpdate(value))
        else:
            self._playbackObserver = None
            self._loopObserver = None

        for name in self.actions:
            self.actions[name].enabled = player != None

    def _playbackUpdate(self, playback):

        self.actions["Stop"].checked = tl.Playback.Stop == playback
        self.actions["Forward"].checked = tl.Playback.Forward == playback
        self.actions["Reverse"].checked = tl.Playback.Reverse == playback

    def _loopUpdate(self, loop):
        self.loopGroup.checked = tl.getLoopEnums().index(loop)
