# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the DJV project.

import opentimelineio as otio
import ftkPy as ftk
import tlRenderPy as tl
import djvPy as djv

import weakref

class Actions:
    """
    This class provides view actions.
    """
    def __init__(self, context, app, mainWindow):

        self._app = weakref.ref(app)
        self._mainWindowWeak = weakref.ref(mainWindow)
        self.actions = {}
        self.actions["Frame"] = ftk.Action(
            "Frame",
            "ViewFrame",
            ftk.KeyShortcut(ftk.Key.Backspace),
            checkedCallback = self._frameCallback)
        self.actions["Frame"].tooltip = "Toggle whether to automatically frame the view."

        self.actions["ZoomReset"] = ftk.Action(
            "Zoom Reset",
            "ViewZoomReset",
            ftk.KeyShortcut(ftk.Key._0),
            callback = lambda: self._mainWindowWeak().getViewport().resetZoom())
        self.actions["ZoomReset"].tooltip = "Reset the view zoom."

        self.actions["ZoomIn"] = ftk.Action(
            "Zoom In",
            "ViewZoomIn",
            ftk.KeyShortcut(ftk.Key.Equals),
            callback = lambda: self._mainWindowWeak().getViewport().zoomIn())
        self.actions["ZoomIn"].tooltip = "Zoom the view in."

        self.actions["ZoomOut"] = ftk.Action(
            "Zoom Out",
            "ViewZoomOut",
            ftk.KeyShortcut(ftk.Key.Minus),
            callback = lambda: self._mainWindowWeak().getViewport().zoomOut())
        self.actions["ZoomOut"].tooltip = "Zoom the view out."

        self.actions["HUD"] = ftk.Action(
            "HUD",
            ftk.KeyShortcut(ftk.Key.H, ftk.commandKeyModifier),
            checkedCallback = self._hudCallback)
        self.actions["HUD"].tooltip = "Toggle the HUD (heads up display)."

        selfWeak = weakref.ref(self)
        self._playerObserver = tl.PlayerObserver(
            app.observePlayer(),
            lambda player: selfWeak()._playerUpdate(player))

        self._frameObserver = ftk.BoolObserver(
            mainWindow.getViewport().observeFrameView,
            lambda value: selfWeak()._frameUpdate(value))

        self._hudObserver = djv.models.HUDOptionsObserver(
            app.getViewportModel().observeHUDOptions,
            lambda value: selfWeak()._hudUpdate(value))

    def _frameCallback(self, value):
        self._mainWindowWeak().getViewport().frameView = value

    def _frameUpdate(self, value):
        self.actions["Frame"].checked = value

    def _hudCallback(self, value):
        model = self._app().getViewportModel()
        options = model.hudOptions
        options.enabled = value
        model.hudOptions = options

    def _hudUpdate(self, value):
        self.actions["HUD"].checked = value.enabled

    def _playerUpdate(self, player):
        self.actions["Frame"].enabled = player != None
        self.actions["ZoomReset"].enabled = player != None
        self.actions["ZoomIn"].enabled = player != None
        self.actions["ZoomOut"].enabled = player != None
