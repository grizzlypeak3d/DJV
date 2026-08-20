# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the DJV project.

import opentimelineio as otio
import ftkPy as ftk
import tlRenderPy as tl
import djvPy as djv

import weakref

class Actions:
    """
    This class provides the frame actions.
    """
    def __init__(self, context, app):

        self._player = None
        self.actions = {}
        items = [
            ("Start", "Start Frame", "FrameStart", tl.TimeAction.Start,
             ftk.KeyShortcut(ftk.Key.Home), "Go to the start frame."),
            ("End", "End Frame", "FrameEnd", tl.TimeAction.End,
             ftk.KeyShortcut(ftk.Key.End), "Go to the end frame."),
            ("Prev", "Previous Frame", "FramePrev", tl.TimeAction.FramePrev,
             ftk.KeyShortcut(ftk.Key.Left), "Go to the previous frame."),
            ("PrevX10", "Previous Frame X10", None, tl.TimeAction.FramePrevX10,
             ftk.KeyShortcut(ftk.Key.Left, ftk.KeyModifier.Shift),
             "Go to the previous frame X10."),
            ("PrevX100", "Previous Frame X100", None, tl.TimeAction.FramePrevX100,
             ftk.KeyShortcut(ftk.Key.Left, ftk.KeyModifier.Control),
             "Go to the previous frame X100."),
            ("Next", "Next Frame", "FrameNext", tl.TimeAction.FrameNext,
             ftk.KeyShortcut(ftk.Key.Right), "Go to the next frame."),
            ("NextX10", "Next Frame X10", None, tl.TimeAction.FrameNextX10,
             ftk.KeyShortcut(ftk.Key.Right, ftk.KeyModifier.Shift),
             "Go to the next frame X10."),
            ("NextX100", "Next Frame X100", None, tl.TimeAction.FrameNextX100,
             ftk.KeyShortcut(ftk.Key.Right, ftk.KeyModifier.Control),
             "Go to the next frame X100."),
        ]
        for name, text, icon, timeAction, shortcut, tooltip in items:
            callback = lambda captured = timeAction: self._timeAction(captured)
            if icon:
                action = ftk.Action(text, icon, shortcut, callback)
            else:
                action = ftk.Action(text, shortcut, callback)
            action.tooltip = tooltip
            self.actions[name] = action

        selfWeak = weakref.ref(self)
        self._playerObserver = tl.PlayerObserver(
            app.observePlayer(),
            lambda player: selfWeak()._playerUpdate(player))

    def _timeAction(self, value):
        if self._player:
            self._player.timeAction(value)

    def _playerUpdate(self, player):
        self._player = player
        for action in self.actions.values():
            action.enabled = player != None
