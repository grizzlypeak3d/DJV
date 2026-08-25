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
    This class provides playback actions.
    """
    def __init__(self, context, app):
        IActions.IActions.__init__(self, context, app, "Playback")

        self._player = None

        # Register the commands.
        self._addCommand(
            "Stop",
            "Stop playback.",
            lambda args, f = Util.weak(self._playerCall): f("stop"))

        self._addCommand(
            "Forward",
            "Start forward playback.",
            lambda args, f = Util.weak(self._playerCall): f("forward"))

        self._addCommand(
            "Reverse",
            "Start reverse playback.",
            lambda args, f = Util.weak(self._playerCall): f("reverse"))

        self._addCommand(
            "Toggle",
            "Toggle playback.",
            lambda args, f = Util.weak(self._playerCall): f("togglePlayback"))

        for name, doc, timeAction in [
            ("JumpBack1s", "Jump back 1 second.", tl.TimeAction.JumpBack1s),
            ("JumpBack10s", "Jump back 10 seconds.", tl.TimeAction.JumpBack10s),
            ("JumpForward1s", "Jump forward 1 second.", tl.TimeAction.JumpForward1s),
            ("JumpForward10s", "Jump forward 10 seconds.", tl.TimeAction.JumpForward10s)]:
            self._addCommand(
                name,
                doc,
                lambda args, captured = timeAction, \
                    f = Util.weak(self._timeAction): f(captured))

        for name, doc, loop in [
            ("Loop", "Loop playback continuously.", tl.Loop.Loop),
            ("Once", "Playback once and stop.", tl.Loop.Once),
            ("PingPong", "Playback forward and reverse continuously.", tl.Loop.PingPong)]:
            self._addCommand(
                name,
                doc,
                lambda args, captured = loop, \
                    f = Util.weak(self._loopCallback): f(captured))

        self._addCommand(
            "SetInPoint",
            "Set the playback in point to the current frame.",
            lambda args, f = Util.weak(self._playerCall): f("setInPoint"))

        self._addCommand(
            "ResetInPoint",
            "Reset the playback in point.",
            lambda args, f = Util.weak(self._playerCall): f("resetInPoint"))

        self._addCommand(
            "SetOutPoint",
            "Set the playback out point to the current frame.",
            lambda args, f = Util.weak(self._playerCall): f("setOutPoint"))

        self._addCommand(
            "ResetOutPoint",
            "Reset the playback out point.",
            lambda args, f = Util.weak(self._playerCall): f("resetOutPoint"))

        # Commands without menu actions, for scripting and automation.
        self._addCommand(
            "Seek",
            "Seek to a frame, relative to the timeline start; "
            "e.g., { \"frame\": 100 }.",
            lambda args, f = Util.weak(self._seekCommand): f(args))

        self._addCommand(
            "InOutRange",
            "Set the playback in/out range from inclusive frames relative "
            "to the timeline start; e.g., { \"in\": 10, \"out\": 50 }.",
            lambda args, f = Util.weak(self._inOutRangeCommand): f(args))

        # Create the actions.
        self.actions["Stop"] = ftk.Action(
            "Stop",
            "PlaybackStop",
            self._command("Stop"))
        self.actions["Forward"] = ftk.Action(
            "Forward",
            "PlaybackForward",
            self._command("Forward"))
        self.actions["Reverse"] = ftk.Action(
            "Reverse",
            "PlaybackReverse",
            self._command("Reverse"))
        self.actions["Toggle"] = ftk.Action(
            "Toggle Playback",
            self._command("Toggle"))
        self.actions["JumpBack1s"] = ftk.Action(
            "Jump Back 1s",
            self._command("JumpBack1s"))
        self.actions["JumpBack10s"] = ftk.Action(
            "Jump Back 10s",
            self._command("JumpBack10s"))
        self.actions["JumpForward1s"] = ftk.Action(
            "Jump Forward 1s",
            self._command("JumpForward1s"))
        self.actions["JumpForward10s"] = ftk.Action(
            "Jump Forward 10s",
            self._command("JumpForward10s"))
        self.actions["Loop"] = ftk.Action(
            "Playback Loop",
            "PlaybackLoop",
            self._command("Loop"))
        self.actions["Once"] = ftk.Action(
            "Playback Once",
            "PlaybackOnce",
            self._command("Once"))
        self.actions["PingPong"] = ftk.Action(
            "Playback Ping-Pong",
            "PlaybackPingPong",
            self._command("PingPong"))
        self.actions["SetInPoint"] = ftk.Action(
            "Set In Point",
            self._command("SetInPoint"))
        self.actions["ResetInPoint"] = ftk.Action(
            "Reset In Point",
            self._command("ResetInPoint"))
        self.actions["SetOutPoint"] = ftk.Action(
            "Set Out Point",
            self._command("SetOutPoint"))
        self.actions["ResetOutPoint"] = ftk.Action(
            "Reset Out Point",
            self._command("ResetOutPoint"))

        # Register the shortcuts.
        self._addShortcut("Stop", ftk.Key.K)
        self._addShortcut("Forward", ftk.Key.L)
        self._addShortcut("Reverse", ftk.Key.J)
        self._addShortcut("Toggle", "Toggle", ftk.KeyShortcut(ftk.Key.Space))
        self._addShortcut("JumpBack1s", ftk.KeyShortcut(ftk.Key.J, ftk.KeyModifier.Shift))
        self._addShortcut("JumpBack10s", ftk.KeyShortcut(ftk.Key.J, ftk.KeyModifier.Control))
        self._addShortcut("JumpForward1s", ftk.KeyShortcut(ftk.Key.L, ftk.KeyModifier.Shift))
        self._addShortcut("JumpForward10s", ftk.KeyShortcut(ftk.Key.L, ftk.KeyModifier.Control))
        self._addShortcut("Loop", "Loop")
        self._addShortcut("Once", "Once")
        self._addShortcut("PingPong", "Ping-Pong")
        self._addShortcut("SetInPoint", ftk.Key.I)
        self._addShortcut("ResetInPoint", ftk.KeyShortcut(ftk.Key.I, ftk.KeyModifier.Shift))
        self._addShortcut("SetOutPoint", ftk.Key.O)
        self._addShortcut("ResetOutPoint", ftk.KeyShortcut(ftk.Key.O, ftk.KeyModifier.Shift))

        self._shortcutsUpdate(self._settingsModel.shortcuts)

        selfWeak = weakref.ref(self)
        self._playerObserver = tl.PlayerObserver(
            app.observePlayer(),
            lambda player: selfWeak()._playerUpdate(player))
        self._loopUpdate(tl.Loop.Loop)

    def _playerCall(self, method):
        if self._player:
            getattr(self._player, method)()

    def _timeAction(self, value):
        if self._player:
            self._player.timeAction(value)

    def _loopCallback(self, value):
        if self._player:
            self._player.loop = value

    def _seekCommand(self, args):
        import json
        if self._player:
            start = self._player.timeRange.start_time
            frame = json.loads(args)["frame"]
            self._player.currentTime = otio.opentime.RationalTime(
                start.value + frame, start.rate)

    def _inOutRangeCommand(self, args):
        import json
        if self._player:
            start = self._player.timeRange.start_time
            parsed = json.loads(args)
            self._player.inOutRange = \
                otio.opentime.TimeRange.range_from_start_end_time_inclusive(
                    otio.opentime.RationalTime(
                        start.value + parsed["in"], start.rate),
                    otio.opentime.RationalTime(
                        start.value + parsed["out"], start.rate))

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
            self._playbackUpdate(tl.Playback.Stop)

        for name in self.actions:
            self.actions[name].enabled = player != None

    def _playbackUpdate(self, playback):
        self.actions["Stop"].checked = tl.Playback.Stop == playback
        self.actions["Forward"].checked = tl.Playback.Forward == playback
        self.actions["Reverse"].checked = tl.Playback.Reverse == playback

    def _loopUpdate(self, loop):
        self.actions["Loop"].checked = tl.Loop.Loop == loop
        self.actions["Once"].checked = tl.Loop.Once == loop
        self.actions["PingPong"].checked = tl.Loop.PingPong == loop
