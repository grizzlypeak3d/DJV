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
    This class provides the frame actions.
    """
    def __init__(self, context, app, mainWindow):
        IActions.IActions.__init__(self, context, app, "Frame")

        self._player = None
        mainWindowWeak = weakref.ref(mainWindow)

        # Register the commands.
        for name, doc, timeAction in [
            ("Start", "Go to the start frame.", tl.TimeAction.Start),
            ("End", "Go to the end frame.", tl.TimeAction.End),
            ("Prev", "Go to the previous frame.", tl.TimeAction.FramePrev),
            ("PrevX10", "Go to the previous frame X10.", tl.TimeAction.FramePrevX10),
            ("PrevX100", "Go to the previous frame X100.", tl.TimeAction.FramePrevX100),
            ("Next", "Go to the next frame.", tl.TimeAction.FrameNext),
            ("NextX10", "Go to the next frame X10.", tl.TimeAction.FrameNextX10),
            ("NextX100", "Go to the next frame X100.", tl.TimeAction.FrameNextX100)]:
            self._addCommand(
                name,
                doc,
                lambda args, captured = timeAction, \
                    f = Util.weak(self._timeAction): f(captured))

        # Jump between the frames that carry a note or a drawing. In a
        # review these are the only frames that matter, and stepping to
        # them by hand over a long timeline is the slow part.
        appWeak = weakref.ref(app)
        self._addCommand(
            "PrevReview",
            "Go to the previous frame with a note or a drawing.",
            lambda args: appWeak().seekReviewMarker(False))

        self._addCommand(
            "NextReview",
            "Go to the next frame with a note or a drawing.",
            lambda args: appWeak().seekReviewMarker(True))

        self._addCommand(
            "FocusCurrent",
            "Set the keyboard focus to the current frame editor.",
            lambda args: mainWindowWeak().focusCurrentFrame())

        # Create the actions.
        self.actions["Start"] = ftk.Action(
            "Go To Start",
            "FrameStart",
            self._command("Start"))
        self.actions["End"] = ftk.Action(
            "Go To End",
            "FrameEnd",
            self._command("End"))
        self.actions["Prev"] = ftk.Action(
            "Previous Frame",
            "FramePrev",
            self._command("Prev"))
        self.actions["PrevX10"] = ftk.Action(
            "Previous Frame X10",
            self._command("PrevX10"))
        self.actions["PrevX100"] = ftk.Action(
            "Previous Frame X100",
            self._command("PrevX100"))
        self.actions["Next"] = ftk.Action(
            "Next Frame",
            "FrameNext",
            self._command("Next"))
        self.actions["NextX10"] = ftk.Action(
            "Next Frame X10",
            self._command("NextX10"))
        self.actions["NextX100"] = ftk.Action(
            "Next Frame X100",
            self._command("NextX100"))
        self.actions["PrevReview"] = ftk.Action(
            "Previous Review",
            "ReviewPrev",
            self._command("PrevReview"))
        self.actions["NextReview"] = ftk.Action(
            "Next Review",
            "ReviewNext",
            self._command("NextReview"))
        self.actions["FocusCurrent"] = ftk.Action(
            "Focus Current Frame",
            self._command("FocusCurrent"))

        # Register the shortcuts.
        self._addShortcut("Start", "Start", ftk.KeyShortcut(ftk.Key.Home))
        self._addShortcut("End", "End", ftk.KeyShortcut(ftk.Key.End))
        self._addShortcut("Prev", "Previous", ftk.KeyShortcut(ftk.Key.Left))
        self._addShortcut("PrevX10", "Previous X10",
            ftk.KeyShortcut(ftk.Key.Left, ftk.KeyModifier.Shift))
        self._addShortcut("PrevX100", "Previous X100",
            ftk.KeyShortcut(ftk.Key.Left, ftk.KeyModifier.Control))
        self._addShortcut("Next", "Next", ftk.KeyShortcut(ftk.Key.Right))
        self._addShortcut("NextX10", "Next X10",
            ftk.KeyShortcut(ftk.Key.Right, ftk.KeyModifier.Shift))
        self._addShortcut("NextX100", "Next X100",
            ftk.KeyShortcut(ftk.Key.Right, ftk.KeyModifier.Control))
        # Shift and Control on the arrows are already taken by the X10
        # and X100 steps.
        self._addShortcut("PrevReview", "Previous review",
            ftk.KeyShortcut(ftk.Key.Left, ftk.KeyModifier.Alt))
        self._addShortcut("NextReview", "Next review",
            ftk.KeyShortcut(ftk.Key.Right, ftk.KeyModifier.Alt))
        self._addShortcut("FocusCurrent", "Focus Current",
            ftk.KeyShortcut(ftk.Key.F, ftk.KeyModifier.Control))

        self._shortcutsUpdate(self._settingsModel.shortcuts)

        selfWeak = weakref.ref(self)
        self._hasMarkers = False
        self._playerObserver = tl.PlayerObserver(
            app.observePlayer(),
            lambda player: selfWeak()._playerUpdate(player))
        self._markersObserver = ftk.IntListObserver(
            app.observeReviewMarkers(),
            lambda markers: selfWeak()._markersUpdate(markers))

    def _timeAction(self, value):
        if self._player:
            self._player.timeAction(value)

    def _playerUpdate(self, player):
        self._player = player
        for action in self.actions.values():
            action.enabled = player != None
        self._markersUpdate(None)

    def _markersUpdate(self, markers):
        if markers is not None:
            self._hasMarkers = len(markers) > 0
        # There is nowhere to jump until a frame carries a note or a
        # drawing.
        enabled = self._player is not None and self._hasMarkers
        self.actions["PrevReview"].enabled = enabled
        self.actions["NextReview"].enabled = enabled
