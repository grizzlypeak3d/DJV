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
    This class provides the color actions.
    """
    def __init__(self, context, app):
        IActions.IActions.__init__(self, context, app, "Color")

        self._app = weakref.ref(app)

        # Register the commands.
        self._addCheckCommand(
            "OCIO",
            "Toggle whether OCIO is enabled.",
            lambda value, f = Util.weak(self._setOCIO): f(value))

        self._addCheckCommand(
            "LUT",
            "Toggle whether the LUT is enabled.",
            lambda value, f = Util.weak(self._setLUT): f(value))

        # Create the actions.
        self.actions["OCIO"] = ftk.Action(
            "OCIO",
            checkedCallback = self._checkCommand("OCIO"))
        self.actions["LUT"] = ftk.Action(
            "LUT",
            checkedCallback = self._checkCommand("LUT"))

        # Register the shortcuts.
        self._addShortcut("OCIO", ftk.KeyShortcut(ftk.Key.N, ftk.KeyModifier.Control))
        self._addShortcut("LUT", ftk.KeyShortcut(ftk.Key.K, ftk.KeyModifier.Control))

        self._shortcutsUpdate(self._settingsModel.shortcuts)

        selfWeak = weakref.ref(self)
        self._ocioObserver = djv.models.OCIOOptionsObserver(
            app.getColorModel().observeOCIOOptions,
            lambda value: setattr(
                selfWeak().actions["OCIO"], "checked", value.enabled))
        self._lutObserver = djv.models.LUTOptionsObserver(
            app.getColorModel().observeLUTOptions,
            lambda value: setattr(
                selfWeak().actions["LUT"], "checked", value.enabled))

    def _setOCIO(self, value):
        model = self._app().getColorModel()
        options = model.ocioOptions
        options.enabled = value
        model.ocioOptions = options

    def _setLUT(self, value):
        model = self._app().getColorModel()
        options = model.lutOptions
        options.enabled = value
        model.lutOptions = options
