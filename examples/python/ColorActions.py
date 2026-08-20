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
    This class provides the color actions.
    """
    def __init__(self, context, app):

        self._app = weakref.ref(app)
        self.actions = {}
        self.actions["OCIO"] = ftk.Action(
            "OCIO",
            ftk.KeyShortcut(ftk.Key.N, ftk.KeyModifier.Control),
            checkedCallback = Util.weak(self._setOCIO))
        self.actions["OCIO"].tooltip = "Toggle whether OCIO is enabled."

        self.actions["LUT"] = ftk.Action(
            "LUT",
            ftk.KeyShortcut(ftk.Key.K, ftk.KeyModifier.Control),
            checkedCallback = Util.weak(self._setLUT))
        self.actions["LUT"].tooltip = "Toggle whether the LUT is enabled."

        selfWeak = weakref.ref(self)
        self._ocioObserver = djv.models.OCIOOptionsObserver(
            app.getColorModel().observeOCIOOptions,
            lambda value: selfWeak()._ocioUpdate(value))
        self._lutObserver = djv.models.LUTOptionsObserver(
            app.getColorModel().observeLUTOptions,
            lambda value: selfWeak()._lutUpdate(value))

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

    def _ocioUpdate(self, options):
        self.actions["OCIO"].checked = options.enabled

    def _lutUpdate(self, options):
        self.actions["LUT"].checked = options.enabled
