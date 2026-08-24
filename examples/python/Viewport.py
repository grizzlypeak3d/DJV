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
    application wraps it instead. The color picker and render items need
    the C++ viewport's picking, so they are not shown here.
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
        self._hudWidgets = {
            djv.models.HUDItem.FileName: self._fileNameLabel,
            djv.models.HUDItem.Time: self._timeLabel,
            djv.models.HUDItem.Info: self._infoLabel,
            djv.models.HUDItem.Cache: self._cacheLabel,
            djv.models.HUDItem.ViewZoom: self._viewZoomLabel,
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
