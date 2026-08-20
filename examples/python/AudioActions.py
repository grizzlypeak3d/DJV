# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the DJV project.

import opentimelineio as otio
import ftkPy as ftk
import tlRenderPy as tl
import djvPy as djv

import weakref

class Actions:
    """
    This class provides the audio actions.
    """
    def __init__(self, context, app):

        appWeak = weakref.ref(app)
        self.actions = {}
        self.actions["VolumeUp"] = ftk.Action(
            "Volume Up",
            ftk.KeyShortcut(ftk.Key.Period),
            lambda: appWeak().getAudioModel().volumeUp())
        self.actions["VolumeUp"].tooltip = "Increase the audio volume."

        self.actions["VolumeDown"] = ftk.Action(
            "Volume Down",
            ftk.KeyShortcut(ftk.Key.Comma),
            lambda: appWeak().getAudioModel().volumeDown())
        self.actions["VolumeDown"].tooltip = "Decrease the audio volume."

        self.actions["Mute"] = ftk.Action(
            "Mute",
            "Mute",
            ftk.KeyShortcut(ftk.Key.M),
            checkedCallback = lambda value:
                setattr(appWeak().getAudioModel(), "mute", value))
        self.actions["Mute"].tooltip = "Toggle the audio mute."

        selfWeak = weakref.ref(self)
        self._muteObserver = ftk.BoolObserver(
            app.getAudioModel().observeMute,
            lambda value: selfWeak()._muteUpdate(value))

    def _muteUpdate(self, value):
        self.actions["Mute"].checked = value
