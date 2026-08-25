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
    This class provides view actions.
    """
    def __init__(self, context, app, mainWindow):
        IActions.IActions.__init__(self, context, app, "View")

        self._app = weakref.ref(app)
        self._mainWindowWeak = weakref.ref(mainWindow)
        mainWindowWeak = self._mainWindowWeak

        # Register the commands.
        self._addCheckCommand(
            "Frame",
            "Frame the view to fit the image.",
            lambda value: setattr(
                mainWindowWeak().getViewport(), "frameView", value))

        self._addCommand(
            "ZoomReset",
            "Reset the view zoom to 1:1.",
            lambda args: mainWindowWeak().getViewport().resetZoom())

        self._addCommand(
            "ZoomIn",
            "Zoom the view in.",
            lambda args: mainWindowWeak().getViewport().zoomIn())

        self._addCommand(
            "ZoomOut",
            "Zoom the view out.",
            lambda args: mainWindowWeak().getViewport().zoomOut())

        self._addCommand(
            "Center",
            "Center the view.",
            lambda args: mainWindowWeak().getViewport().center())

        for name, channel, doc in [
            ("Red", ftk.ChannelDisplay.Red, "Show the red channel."),
            ("Green", ftk.ChannelDisplay.Green, "Show the green channel."),
            ("Blue", ftk.ChannelDisplay.Blue, "Show the blue channel."),
            ("Alpha", ftk.ChannelDisplay.Alpha, "Show the alpha channel.")]:
            self._addCheckCommand(
                name,
                doc,
                lambda value, captured = channel, \
                    f = Util.weak(self._channelCallback): f(captured, value))

        self._addCheckCommand(
            "Negative",
            "Show the image as a negative.",
            lambda value, f = Util.weak(self._displayCallback):
                f("negative", value))

        self._addCheckCommand(
            "MirrorHorizontal",
            "Mirror the image horizontally.",
            lambda value, f = Util.weak(self._mirrorCallback): f("x", value))

        self._addCheckCommand(
            "MirrorVertical",
            "Mirror the image vertically.",
            lambda value, f = Util.weak(self._mirrorCallback): f("y", value))

        self._addCommand(
            "AspectRatio_0",
            "Set the aspect ratio to the default.",
            lambda args, f = Util.weak(self._aspectRatioCallback): f(0))

        self._addCheckCommand(
            "Outline",
            "Toggle the outline.",
            lambda value, f = Util.weak(self._outlineCallback): f(value))

        self._addCheckCommand(
            "Grid",
            "Toggle the grid.",
            lambda value, f = Util.weak(self._gridCallback): f(value))

        self._addCheckCommand(
            "CenterMarker",
            "Toggle the center marker.",
            lambda value, f = Util.weak(self._centerMarkerCallback): f(value))

        self._addCheckCommand(
            "HUD",
            "Toggle the HUD / information display.",
            lambda value, f = Util.weak(self._hudCallback): f(value))

        aspectRatioOptions = djv.models.AspectRatioOptions()
        for i in range(1, len(aspectRatioOptions.options)):
            self._addCommand(
                "AspectRatio_{}".format(i),
                "Set the aspect ratio to {}.".format(
                    tl.getLabel(aspectRatioOptions.options[i])),
                lambda args, captured = i, \
                    f = Util.weak(self._aspectRatioCallback): f(captured))
            self.actions["AspectRatio_{}".format(i)] = ftk.Action(
                "",
                self._command("AspectRatio_{}".format(i)))
            self._addShortcut(
                "AspectRatio_{}".format(i),
                "Custom Aspect Ratio {}".format(i))

        # Create the actions.
        self.actions["Frame"] = ftk.Action(
            "Frame",
            "ViewFrame",
            checkedCallback = self._checkCommand("Frame"))
        self.actions["ZoomReset"] = ftk.Action(
            "Zoom Reset",
            "ViewZoomReset",
            self._command("ZoomReset"))
        self.actions["ZoomIn"] = ftk.Action(
            "Zoom In",
            "ViewZoomIn",
            self._command("ZoomIn"))
        self.actions["ZoomOut"] = ftk.Action(
            "Zoom Out",
            "ViewZoomOut",
            self._command("ZoomOut"))
        self.actions["Center"] = ftk.Action(
            "Center",
            self._command("Center"))
        self.actions["Red"] = ftk.Action(
            "Red Channel",
            checkedCallback = self._checkCommand("Red"))
        self.actions["Green"] = ftk.Action(
            "Green Channel",
            checkedCallback = self._checkCommand("Green"))
        self.actions["Blue"] = ftk.Action(
            "Blue Channel",
            checkedCallback = self._checkCommand("Blue"))
        self.actions["Alpha"] = ftk.Action(
            "Alpha Channel",
            checkedCallback = self._checkCommand("Alpha"))
        self.actions["Negative"] = ftk.Action(
            "Negative",
            checkedCallback = self._checkCommand("Negative"))
        self.actions["MirrorHorizontal"] = ftk.Action(
            "Mirror Horizontal",
            checkedCallback = self._checkCommand("MirrorHorizontal"))
        self.actions["MirrorVertical"] = ftk.Action(
            "Mirror Vertical",
            checkedCallback = self._checkCommand("MirrorVertical"))
        self.actions["AspectRatio_0"] = ftk.Action(
            "Default",
            self._command("AspectRatio_0"))
        self.actions["Outline"] = ftk.Action(
            "Outline",
            checkedCallback = self._checkCommand("Outline"))
        self.actions["Grid"] = ftk.Action(
            "Grid",
            checkedCallback = self._checkCommand("Grid"))
        self.actions["CenterMarker"] = ftk.Action(
            "Center Marker",
            checkedCallback = self._checkCommand("CenterMarker"))
        self.actions["HUD"] = ftk.Action(
            "HUD / Information Display",
            checkedCallback = self._checkCommand("HUD"))

        # Register the shortcuts.
        self._addShortcut("Frame", ftk.Key.Backspace)
        self._addShortcut("ZoomReset", ftk.Key._0)
        self._addShortcut("ZoomIn", ftk.Key.Equals)
        self._addShortcut("ZoomOut", ftk.Key.Minus)
        self._addShortcut("Center", ftk.Key.Backslash)
        self._addShortcut("Red", ftk.Key.R)
        self._addShortcut("Green", ftk.Key.G)
        self._addShortcut("Blue", ftk.Key.B)
        self._addShortcut("Alpha", ftk.Key.A)
        self._addShortcut("Negative", ftk.KeyShortcut(ftk.Key.I, ftk.KeyModifier.Control))
        self._addShortcut("MirrorHorizontal", ftk.Key.H)
        self._addShortcut("MirrorVertical", ftk.Key.V)
        self._addShortcut("AspectRatio_0", "Default Aspect Ratio")
        self._addShortcut("Grid", ftk.KeyShortcut(ftk.Key.G, ftk.KeyModifier.Control))
        self._addShortcut("Outline")
        self._addShortcut("CenterMarker")
        self._addShortcut("HUD", ftk.KeyShortcut(ftk.Key.H, ftk.KeyModifier.Control))

        self._shortcutsUpdate(self._settingsModel.shortcuts)

        selfWeak = weakref.ref(self)
        self._playerObserver = tl.PlayerObserver(
            app.observePlayer(),
            lambda player: selfWeak()._playerUpdate(player))
        self._frameObserver = ftk.BoolObserver(
            mainWindow.getViewport().observeFrameView,
            lambda value: setattr(
                selfWeak().actions["Frame"], "checked", value))
        self._displayOptionsObserver = djv.models.DisplayOptionsObserver(
            app.getViewportModel().observeDisplayOptions,
            lambda value: selfWeak()._displayOptionsUpdate(value))
        self._aspectRatioObserver = djv.models.AspectRatioOptionsObserver(
            app.getViewportModel().observeAspectRatioOptions,
            lambda value: selfWeak()._aspectRatioUpdate(value))
        self._bgOptionsObserver = djv.models.BackgroundOptionsObserver(
            app.getViewportModel().observeBackgroundOptions,
            lambda value: setattr(
                selfWeak().actions["Outline"], "checked", value.outline.enabled))
        self._fgOptionsObserver = djv.models.ForegroundOptionsObserver(
            app.getViewportModel().observeForegroundOptions,
            lambda value: selfWeak()._fgOptionsUpdate(value))
        self._hudObserver = djv.models.HUDOptionsObserver(
            app.getViewportModel().observeHUDOptions,
            lambda value: setattr(
                selfWeak().actions["HUD"], "checked", value.enabled))

    def _channelCallback(self, channel, value):
        model = self._app().getViewportModel()
        options = model.displayOptions
        options.channels = channel if value else ftk.ChannelDisplay.Color
        model.displayOptions = options

    def _displayCallback(self, field, value):
        model = self._app().getViewportModel()
        options = model.displayOptions
        setattr(options, field, value)
        model.displayOptions = options

    def _mirrorCallback(self, axis, value):
        model = self._app().getViewportModel()
        options = model.displayOptions
        mirror = options.mirror
        setattr(mirror, axis, value)
        options.mirror = mirror
        model.displayOptions = options

    def _aspectRatioCallback(self, index):
        model = self._app().getViewportModel()
        options = model.aspectRatioOptions
        options.index = index
        model.aspectRatioOptions = options

    def _outlineCallback(self, value):
        model = self._app().getViewportModel()
        options = model.backgroundOptions
        outline = options.outline
        outline.enabled = value
        options.outline = outline
        model.backgroundOptions = options

    def _gridCallback(self, value):
        model = self._app().getViewportModel()
        options = model.foregroundOptions
        grid = options.grid
        grid.enabled = value
        options.grid = grid
        model.foregroundOptions = options

    def _centerMarkerCallback(self, value):
        model = self._app().getViewportModel()
        options = model.foregroundOptions
        marker = options.centerMarker
        marker.enabled = value
        options.centerMarker = marker
        model.foregroundOptions = options

    def _hudCallback(self, value):
        model = self._app().getViewportModel()
        options = model.hudOptions
        options.enabled = value
        model.hudOptions = options

    def _displayOptionsUpdate(self, value):
        self.actions["Red"].checked = ftk.ChannelDisplay.Red == value.channels
        self.actions["Green"].checked = ftk.ChannelDisplay.Green == value.channels
        self.actions["Blue"].checked = ftk.ChannelDisplay.Blue == value.channels
        self.actions["Alpha"].checked = ftk.ChannelDisplay.Alpha == value.channels
        self.actions["Negative"].checked = value.negative
        self.actions["MirrorHorizontal"].checked = value.mirror.x
        self.actions["MirrorVertical"].checked = value.mirror.y

    def _aspectRatioUpdate(self, value):
        self.actions["AspectRatio_0"].checked = 0 == value.index
        for i in range(1, len(value.options)):
            action = self.actions["AspectRatio_{}".format(i)]
            action.text = tl.getLabel(value.options[i])
            action.checked = i == value.index

    def _fgOptionsUpdate(self, value):
        self.actions["Grid"].checked = value.grid.enabled
        self.actions["CenterMarker"].checked = value.centerMarker.enabled

    def _playerUpdate(self, player):
        # Framing, zooming and centering all describe where an image sits
        # in the view. Media with no video has no image to place.
        video = player is not None and len(player.ioInfo.video) > 0
        self.actions["Frame"].enabled = video
        self.actions["ZoomReset"].enabled = video
        self.actions["ZoomIn"].enabled = video
        self.actions["ZoomOut"].enabled = video
        self.actions["Center"].enabled = video
