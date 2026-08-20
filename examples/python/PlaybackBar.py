# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the DJV project.

import opentimelineio as otio
import ftkPy as ftk
import tlRenderPy as tl
import djvPy as djv

import weakref

class Widget(ftk.IContainer):
    """
    This widget provides playback controls and other time related widgets.
    """
    def __init__(self, context, app, actions, frameActions, parent = None):
        ftk.IContainer.__init__(self, context, "PlaybackBar.Widget", parent)

        self._player = None

        self._playbackToolBar = ftk.ToolBar(context)
        self._playbackToolBar.addAction(actions.actions["Reverse"])
        self._playbackToolBar.addAction(actions.actions["Stop"])
        self._playbackToolBar.addAction(actions.actions["Forward"])

        self._frameToolBar = ftk.ToolBar(context)
        self._frameToolBar.addAction(frameActions.actions["Start"])
        button = self._frameToolBar.addAction(frameActions.actions["Prev"])
        button.repeatClick = True
        button = self._frameToolBar.addAction(frameActions.actions["Next"])
        button.repeatClick = True
        self._frameToolBar.addAction(frameActions.actions["End"])

        self._currentTimeEdit = tl.ui.TimeEdit(context, app.getTimeUnitsModel())
        self._currentTimeEdit.tooltip = "Current time."

        self._durationLabel = tl.ui.TimeLabel(context, app.getTimeUnitsModel())
        self._durationLabel.tooltip = "Playback duration."

        self._speedEdit = ftk.DoubleEdit(context)
        self._speedEdit.range = ftk.RangeD(1.0, 99999.0)
        self._speedEdit.step = 1.0
        self._speedEdit.largeStep = 10.0
        self._speedEdit.tooltip = "Playback speed."

        self._timeUnitsWidget = tl.ui.TimeUnitsWidget(context, app.getTimeUnitsModel())
        self._timeUnitsWidget.tooltip = "Time units."

        self._muteButton = ftk.ToolButton(context)
        self._muteButton.icon = "Volume"
        self._muteButton.checkedIcon = "Mute"
        self._muteButton.tooltip = "Toggle the audio mute."

        self._layout = ftk.HorizontalLayout(context)
        self._layout.marginRole = ftk.SizeRole.MarginInside
        self._playbackToolBar.parent = self._layout
        self._frameToolBar.parent = self._layout
        self._currentTimeEdit.parent = self._layout
        self._durationLabel.parent = self._layout
        self._speedEdit.parent = self._layout
        self._timeUnitsWidget.parent = self._layout
        spacer = ftk.Spacer(context, ftk.Orientation.Horizontal, self._layout)
        spacer.hStretch = ftk.Stretch.Expanding
        self._muteButton.parent = self._layout
        self._setWidget(self._layout)

        self._currentTimeEdit.setCallback(self._currentTimeCallback)
        self._speedEdit.setCallback(self._speedCallback)
        appWeak = weakref.ref(app)
        self._muteButton.setCheckedCallback(
            lambda value: setattr(appWeak().getAudioModel(), "mute", value))

        selfWeak = weakref.ref(self)
        self._playerObserver = tl.PlayerObserver(
            app.observePlayer(),
            lambda player: selfWeak()._playerUpdate(player))
        self._muteObserver = ftk.BoolObserver(
            app.getAudioModel().observeMute,
            lambda value: selfWeak()._muteUpdate(value))

    def _currentTimeCallback(self, value):
        if self._player:
            self._player.currentTime = value

    def _currentTimeUpdate(self, value):
        self._currentTimeEdit.value = value

    def _speedCallback(self, value):
        if self._player:
            self._player.speed = value

    def _speedUpdate(self, value):
        self._speedEdit.value = value

    def _muteUpdate(self, value):
        self._muteButton.checked = value

    def _playerUpdate(self, player):
        self._player = player
        if player:
            self._durationLabel.value = player.duration
            selfWeak = weakref.ref(self)
            self._currentTimeObserver = tl.RationalTimeObserver(
                player.observeCurrentTime,
                lambda value: selfWeak()._currentTimeUpdate(value))
            self._speedObserver = ftk.DoubleObserver(
                player.observeSpeed,
                lambda value: selfWeak()._speedUpdate(value))
        else:
            self._currentTimeEdit.value = None
            self._durationLabel.value = None
            self._speedEdit.value = 1.0
            self._currentTimeObserver = None
            self._speedObserver = None
        self._playbackToolBar.enabled = player != None
        self._frameToolBar.enabled = player != None
        self._currentTimeEdit.enabled = player != None
        self._durationLabel.enabled = player != None
        self._speedEdit.enabled = player != None
