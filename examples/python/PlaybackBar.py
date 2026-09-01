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
    This widget provides playback controls and other time related widgets.
    """
    def __init__(self, context, app, actions, frameActions, reviewActions, parent = None):
        ftk.IContainer.__init__(self, context, "PlaybackBar.Widget", parent)

        self._appWeak = weakref.ref(app)
        self._player = None
        # Whether media time means anything across the whole timeline,
        # which it only does when the timeline plays one media through.
        self._mediaTime = False
        # Where the frame shuttle started, unset until it is grabbed.
        self._startTime = None
        self._speedPopup = None
        self._audioPopup = None

        self._speedModel = ftk.DoubleModel()
        self._speedModel.range = ftk.RangeD(0.0, 1000000.0)
        self._speedModel.step = 1.0
        self._speedModel.largeStep = 10.0

        self._buttons = {}
        for name in ["Reverse", "Stop", "Forward"]:
            self._buttons[name] = ftk.ToolButton(context, actions.actions[name])

        self._loopWidget = tl.ui.PlaybackLoopWidget(context)

        self._playbackShuttle = ftk.ShuttleWidget(context)
        self._playbackShuttle.tooltip = \
            "Playback shuttle. Click and drag to change playback speed."

        for name in ["Start", "Prev", "Next", "End"]:
            self._buttons[name] = ftk.ToolButton(
                context, frameActions.actions[name])
        for name in ["PrevFrame", "NextFrame"]:
            self._buttons[name] = ftk.ToolButton(
                context, reviewActions.actions[name])
        self._buttons["Prev"].repeatClick = True
        self._buttons["Next"].repeatClick = True

        self._frameShuttle = ftk.ShuttleWidget(context)
        self._frameShuttle.tooltip = \
            "Frame shuttle. Click and drag to change the current frame."

        self._currentTimeEdit = tl.ui.TimeEdit(context, app.getTimeUnitsModel())
        self._currentTimeEdit.tooltip = "Current time."

        self._durationLabel = tl.ui.TimeLabel(context, app.getTimeUnitsModel())
        self._durationLabel.setMarginRole(ftk.SizeRole.MarginInside)
        self._durationLabel.tooltip = \
            "Duration of the timeline or the in/out range if set."

        self._timeUnitsWidget = tl.ui.TimeUnitsWidget(context, app.getTimeUnitsModel())
        self._timeUnitsWidget.tooltip = "Time units."

        self._speedButton = ftk.ToolButton(context)
        self._speedButton.popupIcon = True
        self._speedButton.tooltip = "Playback speed."

        self._audioLabel = ftk.Label(context)
        self._audioLabel.font = ftk.FontType.Mono
        self._audioLabel.hMarginRole = ftk.SizeRole.MarginInside
        self._audioLabel.tooltip = "Audio volume."

        self._audioButton = ftk.ToolButton(context)
        self._audioButton.icon = "Volume"
        self._audioButton.popupIcon = True
        self._audioButton.tooltip = "Audio controls."

        self._indicator = djv.ui.StatusIndicator(
            context,
            app.getViewportModel(),
            app.getColorModel(),
            app.getAudioModel())

        # Grouped like the top tool bar: the groups separated by
        # dividers, with the tool bar's spacing between them.
        self._layout = ftk.HorizontalLayout(context)
        self._layout.marginRole = ftk.SizeRole.MarginInside
        hLayout = ftk.HorizontalLayout(context, self._layout)
        hLayout.spacingRole = ftk.SizeRole.Spacing
        hLayout2 = ftk.HorizontalLayout(context, hLayout)
        hLayout2.spacingRole = ftk.SizeRole._None
        self._buttons["Reverse"].parent = hLayout2
        self._buttons["Stop"].parent = hLayout2
        self._buttons["Forward"].parent = hLayout2
        self._loopWidget.parent = hLayout2
        self._playbackShuttle.parent = hLayout2
        ftk.Divider(context, ftk.Orientation.Horizontal, hLayout)
        hLayout2 = ftk.HorizontalLayout(context, hLayout)
        hLayout2.spacingRole = ftk.SizeRole._None
        self._buttons["Start"].parent = hLayout2
        self._buttons["Prev"].parent = hLayout2
        self._buttons["Next"].parent = hLayout2
        self._buttons["End"].parent = hLayout2
        self._frameShuttle.parent = hLayout2
        self._reviewDivider = ftk.Divider(
            context, ftk.Orientation.Horizontal, hLayout)
        # The review jumps sit with the frame navigation: they are the
        # same gesture, on the frames that carry a note or a drawing --
        # and the group is only there when such a frame exists.
        self._reviewLayout = ftk.HorizontalLayout(context, hLayout)
        self._reviewLayout.spacingRole = ftk.SizeRole._None
        self._buttons["PrevFrame"].parent = self._reviewLayout
        self._buttons["NextFrame"].parent = self._reviewLayout
        ftk.Divider(context, ftk.Orientation.Horizontal, hLayout)
        hLayout2 = ftk.HorizontalLayout(context, hLayout)
        hLayout2.spacingRole = ftk.SizeRole.SpacingSmall
        self._currentTimeEdit.parent = hLayout2
        self._durationLabel.parent = hLayout2
        self._timeUnitsWidget.parent = hLayout2
        self._speedButton.parent = hLayout2
        spacer = ftk.Spacer(context, ftk.Orientation.Horizontal, self._layout)
        spacer.hStretch = ftk.Stretch.Expanding
        hLayout2 = ftk.HorizontalLayout(context, self._layout)
        hLayout2.spacingRole = ftk.SizeRole.SpacingSmall
        self._audioLabel.parent = hLayout2
        self._audioButton.parent = hLayout2
        self._indicator.parent = self._layout
        self._setWidget(self._layout)

        self._loopWidget.setCallback(Util.weak(self._loopCallback))
        self._playbackShuttle.setActiveCallback(
            Util.weak(self._playbackShuttleActive))
        self._playbackShuttle.setCallback(Util.weak(self._playbackShuttleCallback))
        self._frameShuttle.setActiveCallback(Util.weak(self._frameShuttleActive))
        self._frameShuttle.setCallback(Util.weak(self._frameShuttleCallback))
        self._currentTimeEdit.setCallback(Util.weak(self._currentTimeCallback))
        self._speedButton.setPressedCallback(Util.weak(self._showSpeedPopup))
        self._audioButton.setPressedCallback(Util.weak(self._showAudioPopup))

        selfWeak = weakref.ref(self)
        self._reviewMarkersObserver = ftk.IntListObserver(
            app.observeReviewMarkers(),
            lambda markers: selfWeak()._reviewMarkersUpdate(markers))
        self._playerObserver = tl.PlayerObserver(
            app.observePlayer(),
            lambda player: selfWeak()._playerUpdate(player))
        self._speedModelObserver = ftk.DoubleObserver(
            self._speedModel.observeValue(),
            lambda value: selfWeak()._speedModelUpdate(value))
        self._volumeObserver = ftk.FloatObserver(
            app.getAudioModel().observeVolume,
            lambda value: selfWeak()._volumeUpdate(value))
        self._muteObserver = ftk.BoolObserver(
            app.getAudioModel().observeMute,
            lambda value: selfWeak()._muteUpdate(value))

    def _reviewMarkersUpdate(self, markers):
        visible = len(markers) > 0
        self._reviewDivider.setVisible(visible)
        self._reviewLayout.setVisible(visible)

    def focusCurrentFrame(self):
        if self._currentTimeEdit.enabled:
            self._currentTimeEdit.takeKeyFocus()
            self._currentTimeEdit.selectAll()

    def _loopCallback(self, value):
        if self._player:
            self._player.loop = value

    def _playbackShuttleActive(self, value):
        if self._player:
            if value:
                if self._player.isStopped:
                    self._player.forward()
            else:
                self._player.speedMult = 1.0

    def _playbackShuttleCallback(self, value):
        if self._player:
            self._player.speedMult = 1.0 + value / 10.0

    def _frameShuttleActive(self, value):
        if self._player:
            self._player.stop()
            self._startTime = self._player.currentTime

    def _frameShuttleCallback(self, value):
        if self._player and self._startTime is not None:
            self._player.currentTime = otio.opentime.RationalTime(
                self._startTime.value + value,
                self._startTime.rate)

    def _currentTimeCallback(self, value):
        if self._player:
            self._player.stop()
            self._player.currentTime = value
            self._currentTimeEdit.value = self._player.currentTime

    def _showSpeedPopup(self):
        if self._speedPopup is None:
            defaultSpeed = self._player.defaultSpeed if self._player else 0.0
            self._speedPopup = djv.ui.SpeedPopup(
                self.context, self._speedModel, defaultSpeed)
            self._speedPopup.open(self.window, self._speedButton.geometry)
            selfWeak = weakref.ref(self)
            def callback(value):
                widget = selfWeak()
                if widget is not None:
                    if widget._player:
                        widget._player.speed = value
                    widget._speedPopup.close()
            self._speedPopup.setCallback(callback)
            self._speedPopup.setCloseCallback(
                lambda: selfWeak() and setattr(
                    selfWeak(), "_speedPopup", None))
        else:
            self._speedPopup.close()
            self._speedPopup = None

    def _showAudioPopup(self):
        if self._audioPopup is None:
            self._audioPopup = djv.ui.AudioPopup(
                self.context, self._appWeak().getAudioModel())
            self._audioPopup.open(self.window, self._audioButton.geometry)
            selfWeak = weakref.ref(self)
            self._audioPopup.setCloseCallback(
                lambda: selfWeak() and setattr(
                    selfWeak(), "_audioPopup", None))
        else:
            self._audioPopup.close()
            self._audioPopup = None

    def _currentTimeUpdate(self, value):
        self._currentTimeEdit.value = value

    def _speedModelUpdate(self, value):
        if self._player:
            self._player.speed = value

    def _speedUpdate(self, value):
        self._speedModel.value = value

    def _actualSpeedUpdate(self, value):
        video = self._player and len(self._player.ioInfo.video) > 0
        self._speedButton.text = "{:.2f}".format(value) if video else ""

    def _loopUpdate(self, value):
        self._loopWidget.loop = value

    def _mediaDuration(self, value):
        duration = value.duration
        if self._player and self._mediaTime and duration.value > 0:
            # Counted in the media's frames rather than the player's, so
            # that a sequence with frames left out still reads as the
            # range it covers. Both ends are mapped because the in/out
            # range may be a part of it.
            timeline = self._player.timeline
            first = timeline.getMediaTime(value.start_time)
            last = timeline.getMediaTime(value.end_time_inclusive())
            if first is not None and last is not None:
                duration = otio.opentime.RationalTime(
                    last.value - first.value + 1.0, last.rate)
        return duration

    def _toMedia(self, value):
        if self._player:
            time = self._player.timeline.getMediaTime(value)
            if time is not None:
                return time
        return value

    def _fromMedia(self, value):
        if self._player:
            # Read against where playback is, which is the clip the
            # person typing is looking at.
            time = self._player.timeline.getTimelineTime(
                self._player.currentTime, value)
            if time is not None:
                return time
        return value

    def _inOutRangeUpdate(self, value):
        self._durationLabel.value = self._mediaDuration(value)

    def _volumeUpdate(self, value):
        self._audioLabel.text = "{:3d}%".format(int(value * 100.0))

    def _muteUpdate(self, value):
        self._audioButton.icon = "Mute" if value else "Volume"

    def _playerUpdate(self, player):
        self._player = player
        self._mediaTime = \
            player.timeline.isMediaTimeContinuous() if player else False
        # The counter names the frame in the media's own time, which is
        # not the player's when frames have been left out. Only where
        # that is one number for the whole timeline.
        timeMap = tl.ui.TimeMap()
        if self._mediaTime:
            # The map must always answer with a time, including after the
            # widget is gone.
            mapWeak = weakref.ref(self)
            def toMedia(value):
                widget = mapWeak()
                return widget._toMedia(value) if widget else value
            def fromMedia(value):
                widget = mapWeak()
                return widget._fromMedia(value) if widget else value
            timeMap.toMedia = toMedia
            timeMap.fromMedia = fromMedia
        self._currentTimeEdit.setTimeMap(timeMap)
        if player:
            selfWeak = weakref.ref(self)
            self._speedObserver = ftk.DoubleObserver(
                player.observeSpeed,
                lambda value: selfWeak()._speedUpdate(value))
            self._actualSpeedObserver = ftk.DoubleObserver(
                player.observeActualSpeed,
                lambda value: selfWeak()._actualSpeedUpdate(value))
            self._loopObserver = tl.LoopObserver(
                player.observeLoop,
                lambda value: selfWeak()._loopUpdate(value))
            self._currentTimeObserver = tl.RationalTimeObserver(
                player.observeCurrentTime,
                lambda value: selfWeak()._currentTimeUpdate(value))
            self._inOutRangeObserver = tl.TimeRangeObserver(
                player.observeInOutRange,
                lambda value: selfWeak()._inOutRangeUpdate(value))
        else:
            self._loopWidget.loop = tl.Loop.Loop
            self._currentTimeEdit.value = None
            self._durationLabel.value = None
            self._speedModel.value = 0.0
            self._speedButton.text = "{:.2f}".format(0.0)
            self._speedObserver = None
            self._actualSpeedObserver = None
            self._loopObserver = None
            self._currentTimeObserver = None
            self._inOutRangeObserver = None

        # A file with no video is timed in audio samples, so what would
        # be shown as a speed is the sample rate and there are no frames
        # to shuttle through.
        video = player is not None and len(player.ioInfo.video) > 0
        self._loopWidget.enabled = player != None
        self._playbackShuttle.enabled = player != None
        self._frameShuttle.enabled = video
        self._currentTimeEdit.enabled = player != None
        self._durationLabel.enabled = player != None
        self._speedButton.enabled = video
