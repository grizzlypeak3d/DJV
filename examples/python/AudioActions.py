# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the DJV project.

import opentimelineio as otio
import ftkPy as ftk
import tlRenderPy as tl
import djvPy as djv

import IActions

import weakref

class Actions(IActions.IActions):
    """
    This class provides the audio actions.
    """
    def __init__(self, context, app):
        IActions.IActions.__init__(self, context, app, "Audio")

        appWeak = weakref.ref(app)

        # Register the commands.
        self._addCommand(
            "VolumeUp",
            "Increase the audio volume.",
            lambda args: appWeak().getAudioModel().volumeUp())

        self._addCommand(
            "VolumeDown",
            "Decrease the audio volume.",
            lambda args: appWeak().getAudioModel().volumeDown())

        self._addCheckCommand(
            "Mute",
            "Toggle the audio mute.",
            lambda value: setattr(appWeak().getAudioModel(), "mute", value))

        # Create the actions.
        self.actions["VolumeUp"] = ftk.Action(
            "Volume Up",
            self._command("VolumeUp"))
        self.actions["VolumeDown"] = ftk.Action(
            "Volume Down",
            self._command("VolumeDown"))
        self.actions["Mute"] = ftk.Action(
            "Mute",
            "Mute",
            checkedCallback = self._checkCommand("Mute"))

        # Register the shortcuts.
        self._addShortcut("VolumeUp", ftk.Key.Period)
        self._addShortcut("VolumeDown", ftk.Key.Comma)
        self._addShortcut("Mute", ftk.Key.M)

        self._shortcutsUpdate(self._settingsModel.shortcuts)

        selfWeak = weakref.ref(self)
        self._muteObserver = ftk.BoolObserver(
            app.getAudioModel().observeMute,
            lambda value: setattr(
                selfWeak().actions["Mute"], "checked", value))
