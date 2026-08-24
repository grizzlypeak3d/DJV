# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the DJV project.

import opentimelineio as otio
import ftkPy as ftk
import tlRenderPy as tl
import djvPy as djv

import Util

import weakref

class Widget(ftk.IContainer):
    """
    This widget stacks the HUD (heads up display) over the viewport. The
    C++ application subclasses the viewport for this; the Python
    application wraps it instead.
    """
    def __init__(self, context, app, parent = None):
        ftk.IContainer.__init__(self, context, "Viewport.Widget", parent)

        self._appWeak = weakref.ref(app)
        self._player = None
        self._fps = 0.0
        self._droppedFrames = 0
        self._currentTime = None
        self._cacheInfo = None

        self._viewport = tl.ui.Viewport(context)
        self._setWidget(self._viewport)

        self._fileNameLabel = ftk.Label(context)
        self._fileNameLabel.font = ftk.FontType.Mono
        self._fileNameLabel.marginRole = ftk.SizeRole.MarginSmall
        self._timeLabel = ftk.Label(context)
        self._timeLabel.font = ftk.FontType.Mono
        self._timeLabel.marginRole = ftk.SizeRole.MarginSmall
        self._infoLabel = ftk.Label(context)
        self._infoLabel.font = ftk.FontType.Mono
        self._infoLabel.marginRole = ftk.SizeRole.MarginSmall
        self._cacheLabel = ftk.Label(context)
        self._cacheLabel.font = ftk.FontType.Mono
        self._cacheLabel.marginRole = ftk.SizeRole.MarginSmall
        self._viewZoomLabel = ftk.Label(context)
        self._viewZoomLabel.font = ftk.FontType.Mono
        self._viewZoomLabel.marginRole = ftk.SizeRole.MarginSmall
        self._renderLabel = ftk.Label(context)
        self._renderLabel.font = ftk.FontType.Mono
        self._renderLabel.marginRole = ftk.SizeRole.MarginSmall
        self._colorPickerSwatch = ftk.ColorSwatch(context)
        self._colorPickerSwatch.vAlign = ftk.VAlign.Center
        self._colorPickerLabel = ftk.Label(context)
        self._colorPickerLabel.font = ftk.FontType.Mono
        self._colorPickerLabel.marginRole = ftk.SizeRole.MarginSmall
        self._colorPickerLayout = ftk.HorizontalLayout(context)
        self._colorPickerLayout.spacingRole = ftk.SizeRole.SpacingSmall
        self._colorPickerSwatch.parent = self._colorPickerLayout
        self._colorPickerLabel.parent = self._colorPickerLayout
        self._hudWidgets = {
            djv.models.HUDItem.FileName: self._fileNameLabel,
            djv.models.HUDItem.Time: self._timeLabel,
            djv.models.HUDItem.Info: self._infoLabel,
            djv.models.HUDItem.Cache: self._cacheLabel,
            djv.models.HUDItem.ViewZoom: self._viewZoomLabel,
            djv.models.HUDItem.Render: self._renderLabel,
            djv.models.HUDItem.ColorPicker: self._colorPickerLayout,
        }

        # One layout per corner, arranged like the C++ viewport's HUD.
        self._hudLayout = ftk.VerticalLayout(context)
        self._hudLayout.parent = self
        self._hudLayout.marginRole = ftk.SizeRole.MarginSmall
        self._hudLayout.spacingRole = ftk.SizeRole._None
        self._corners = {}
        for pos in [djv.models.HUDPos.TopLeft, djv.models.HUDPos.TopRight,
                    djv.models.HUDPos.BottomLeft, djv.models.HUDPos.BottomRight]:
            layout = ftk.VerticalLayout(context)
            layout.marginRole = ftk.SizeRole.MarginInside
            layout.spacingRole = ftk.SizeRole._None
            layout.backgroundColor = ftk.ColorRole.Overlay
            self._corners[pos] = layout
        self._corners[djv.models.HUDPos.TopLeft].vAlign = ftk.VAlign.Top
        self._corners[djv.models.HUDPos.TopRight].vAlign = ftk.VAlign.Top
        self._corners[djv.models.HUDPos.BottomLeft].vAlign = ftk.VAlign.Bottom
        self._corners[djv.models.HUDPos.BottomRight].vAlign = ftk.VAlign.Bottom

        topLayout = ftk.HorizontalLayout(context, self._hudLayout)
        topLayout.spacingRole = ftk.SizeRole.SpacingSmall
        topLayout.vAlign = ftk.VAlign.Top
        self._corners[djv.models.HUDPos.TopLeft].parent = topLayout
        spacer = ftk.Spacer(context, ftk.Orientation.Horizontal, topLayout)
        spacer.hStretch = ftk.Stretch.Expanding
        self._corners[djv.models.HUDPos.TopRight].parent = topLayout

        spacer = ftk.Spacer(context, ftk.Orientation.Vertical, self._hudLayout)
        spacer.vStretch = ftk.Stretch.Expanding

        bottomLayout = ftk.HorizontalLayout(context, self._hudLayout)
        bottomLayout.spacingRole = ftk.SizeRole.SpacingSmall
        bottomLayout.vAlign = ftk.VAlign.Bottom
        self._corners[djv.models.HUDPos.BottomLeft].parent = bottomLayout
        spacer = ftk.Spacer(context, ftk.Orientation.Horizontal, bottomLayout)
        spacer.hStretch = ftk.Stretch.Expanding
        self._corners[djv.models.HUDPos.BottomRight].parent = bottomLayout

        selfWeak = weakref.ref(self)
        self._fpsObserver = ftk.DoubleObserver(
            self._viewport.observeFPS,
            lambda value: selfWeak()._fpsUpdate(value))
        self._droppedFramesObserver = ftk.SizeTObserver(
            self._viewport.observeDroppedFrames,
            lambda value: selfWeak()._droppedFramesUpdate(value))
        self._zoomObserver = ftk.DoubleObserver(
            self._viewport.observeZoom,
            lambda value: selfWeak()._zoomUpdate(value))
        self._hudOptionsObserver = djv.models.HUDOptionsObserver(
            app.getViewportModel().observeHUDOptions,
            lambda value: selfWeak()._hudOptionsUpdate(value))
        self._pickObserver = tl.ui.OptionalV2IObserver(
            self._viewport.observePick,
            lambda value: selfWeak()._pickUpdate())
        self._colorSampleObserver = tl.ui.OptionalColor4FObserver(
            self._viewport.observeColorSample,
            lambda value: selfWeak()._pickUpdate())
        self._mouseSettingsObserver = djv.models.MouseSettingsObserver(
            app.getSettingsModel().observeMouse,
            lambda value: selfWeak()._mouseSettingsUpdate(value))

    def getViewport(self):
        return self._viewport

    def setPlayer(self, player):
        self._player = player
        self._viewport.player = player
        if player:
            selfWeak = weakref.ref(self)
            self._currentTimeObserver = tl.RationalTimeObserver(
                player.observeCurrentTime,
                lambda value: selfWeak()._currentTimeUpdate(value))
            self._cacheInfoObserver = tl.PlayerCacheInfoObserver(
                player.observeCacheInfo,
                lambda value: selfWeak()._cacheInfoUpdate(value))
        else:
            self._currentTime = None
            self._cacheInfo = None
            self._currentTimeObserver = None
            self._cacheInfoObserver = None
        self._hudUpdate()

    def setGeometry(self, value):
        ftk.IContainer.setGeometry(self, value)
        self._hudLayout.setGeometry(value)

    def _fpsUpdate(self, value):
        self._fps = value
        self._hudUpdate()

    def _droppedFramesUpdate(self, value):
        self._droppedFrames = value
        self._hudUpdate()

    def _zoomUpdate(self, value):
        self._viewZoomLabel.text = "Zoom: {:.2f}".format(value)

    def _mouseSettingsUpdate(self, settings):
        # The mouse bindings go to the viewport like the C++ application's.
        none = djv.models.MouseActionBinding(
            ftk.MouseButton._None, ftk.KeyModifier._None)
        b = settings.bindings.get(djv.models.MouseAction.PanView, none)
        self._viewport.setPanBinding(b.button, b.modifier)
        b = settings.bindings.get(djv.models.MouseAction.CompareWipe, none)
        self._viewport.setWipeBinding(b.button, b.modifier)
        b = settings.bindings.get(djv.models.MouseAction.Pick, none)
        self._viewport.setPickBinding(b.button, b.modifier)

    def _pickUpdate(self):
        colorSample = self._viewport.observeColorSample.get()
        pick = self._viewport.observePick.get()
        self._colorPickerSwatch.color = \
            colorSample if colorSample is not None else ftk.Color4F()
        # The HUD sits under the pointer, so the line keeps its field
        # widths whether or not there is a sample, the same as the C++
        # application's.
        if colorSample is not None and pick is not None:
            text = "Color: {:5.2f} {:5.2f} {:5.2f} {:5.2f}, Pixel: {:4d}, {:4d}".format(
                colorSample.r, colorSample.g, colorSample.b, colorSample.a,
                pick.x, pick.y)
        else:
            text = "Color: {0:>5} {0:>5} {0:>5} {0:>5}, Pixel: {1:>4}, {1:>4}".format(
                "-", "-")
        self._colorPickerLabel.text = text

    def _currentTimeUpdate(self, value):
        self._currentTime = value
        self._hudUpdate()

    def _cacheInfoUpdate(self, value):
        self._cacheInfo = value
        self._hudUpdate()

    def _hudOptionsUpdate(self, options):
        self._hudLayout.setVisible(options.enabled)
        for item, widget in self._hudWidgets.items():
            pos = options.items.get(item, djv.models.HUDPos._None)
            widget.parent = self._corners.get(pos, None)

    def _hudUpdate(self):

        player = self._player
        path = player.path if player else None
        self._fileNameLabel.text = path.fileName if path else "(No file)"

        info = []
        ioInfo = player.ioInfo if player else None
        if ioInfo and ioInfo.video:
            videoInfo = ioInfo.video[0]
            info.append("V: {}x{}:{:.2f} {}".format(
                videoInfo.size.w,
                videoInfo.size.h,
                videoInfo.aspect,
                ftk.to_string(videoInfo.type)))
        if ioInfo and ioInfo.audio.isValid:
            info.append("A: {} {} {}".format(
                ioInfo.audio.channelCount,
                tl.to_string(ioInfo.audio.type),
                ioInfo.audio.sampleRate))
        self._infoLabel.text = ", ".join(info)
        self._infoLabel.setVisible(len(info) > 0)

        s = ""
        if self._currentTime is not None:
            app = self._appWeak()
            s = app.getTimeUnitsModel().getLabel(self._currentTime)
        if ioInfo and ioInfo.video:
            self._timeLabel.text = "Time: {}, {:6.2f} FPS, {:3d} dropped".format(
                s, self._fps, self._droppedFrames)
        else:
            self._timeLabel.text = "Time: {}".format(s)

        cache = []
        if self._cacheInfo:
            if ioInfo and ioInfo.video:
                cache.append("{:3d}% V".format(int(self._cacheInfo.videoPercentage)))
            if ioInfo and ioInfo.audio.isValid:
                cache.append("{:3d}% A".format(int(self._cacheInfo.audioPercentage)))
        s = "Cache: {}".format(", ".join(cache)) if cache else ""
        self._cacheLabel.text = s
        self._cacheLabel.setVisible(len(s) > 0)

        # What is actually rendered. The aspect ratio overrides are not
        # wrapped yet, so only the media's own pixel aspect ratio applies.
        s = ""
        if ioInfo and ioInfo.video:
            videoInfo = ioInfo.video[0]
            par = videoInfo.pixelAspectRatio
            w = int(round(videoInfo.size.w * par))
            h = videoInfo.size.h
            if w > 0 and h > 0:
                s = "Render: {}x{}:{:.2f}".format(w, h, w / h)
                if abs(par - 1.0) > 0.001:
                    s += ", PAR: {:.2f}".format(par)
        self._renderLabel.text = s
        self._renderLabel.setVisible(len(s) > 0)
