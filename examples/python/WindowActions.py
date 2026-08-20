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
    This class provides window actions.
    """
    def __init__(self, context, app, mainWindow):

        self._mainWindowWeak = weakref.ref(mainWindow)
        self.actions = {}
        self.actions["FullScreen"] = ftk.Action(
            "Full Screen",
            "WindowFullScreen",
            ftk.KeyShortcut(ftk.Key.U, ftk.commandKeyModifier),
            checkedCallback = Util.weak(self._fullScreenCallback))
        self.actions["FullScreen"].tooltip = "Toggle the window full screen."

    def _fullScreenCallback(self, value):
        if self._mainWindowWeak:
            self._mainWindowWeak().fullScreen = value
