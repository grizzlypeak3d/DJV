# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the DJV project.

import opentimelineio as otio
import ftkPy as ftk
import tlRenderPy as tl
import djvPy as djv

import weakref

class Widget(ftk.IContainer):
    """
    This widget displays errors and other information.
    """
    def __init__(self, context, app, mainWindow, parent = None):
        ftk.IContainer.__init__(self, context, "StatusBar.Widget", parent)

        self._logLabel = ftk.Label(context)
        self._logLabel.hStretch = ftk.Stretch.Expanding
        self._logLabel.marginRole = ftk.SizeRole.MarginInside

        self._infoLabel = ftk.Label(context)
        self._infoLabel.marginRole = ftk.SizeRole.MarginInside

        self._layout = ftk.HorizontalLayout(context)
        self._layout.spacingRole = ftk.SizeRole.SpacingSmall
        self._logLabel.parent = self._layout
        ftk.Divider(context, ftk.Orientation.Horizontal, self._layout)
        self._infoLabel.parent = self._layout
        self._setWidget(self._layout)

        selfWeak = weakref.ref(self)
        self._logObserver = ftk.LogItemListObserver(
            context.getSystemByName("ftk::LogSystem").observeLogItems,
            lambda logItems: selfWeak()._logUpdate(logItems))

        self._playerObserver = tl.PlayerObserver(
            app.observePlayer(),
            lambda player: selfWeak()._infoUpdate(player))

        self._logTimer = ftk.Timer(context)

    def _logUpdate(self, logItems):
        if logItems:
            text = ""
            for item in logItems:
                pieces = []
                if ftk.LogType.Error == item.type:
                    pieces = item.message.split('\n')
                if pieces:
                    text = pieces[0]
            self._logLabel.text = text
            if text:
                self._logTimer.start(5.0, self._clearLog)

    def _clearLog(self):
        self._logLabel.text = ""

    def _infoUpdate(self, player):
        text = []
        tooltip = []
        if player:
            path = player.path
            text.append(path.fileName)
            tooltip.append(path.get())

            ioInfo = player.ioInfo
            if ioInfo.video:
                videoInfo = ioInfo.video[0]
                s = "video: {}x{}:{:.2f} {}".format(
                    videoInfo.size.w,
                    videoInfo.size.h,
                    videoInfo.aspect,
                    ftk.to_string(videoInfo.type))
                text.append(s)
                tooltip.append(s)

            if ioInfo.audio.isValid:
                s = "audio: {} {} {}".format(
                    ioInfo.audio.channelCount,
                    tl.to_string(ioInfo.audio.type),
                    ioInfo.audio.sampleRate)
                text.append(s)
                tooltip.append(s)

        self._infoLabel.text = ", ".join(text)
        self._infoLabel.tooltip = "\n".join(tooltip)
