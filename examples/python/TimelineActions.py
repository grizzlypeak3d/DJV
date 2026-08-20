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
    This class provides the timeline actions.
    """
    def __init__(self, context, app):

        self._app = weakref.ref(app)
        self.actions = {}
        items = [
            ("Minimize", "minimize", "Minimize the timeline."),
            ("FrameView", "frameView", "Frame the timeline view."),
            ("ScrollBars", "scrollBars", "Toggle the scroll bars."),
            ("AutoScroll", "autoScroll",
             "Automatically scroll the timeline to the current frame."),
            ("StopOnScrub", "stopOnScrub",
             "Stop playback when scrubbing the timeline."),
            ("TrackMedia", "trackMedia",
             "Toggle the timeline video thumbnails and audio waveforms."),
            ("Thumbnails", "thumbnails",
             "Toggle the timeline video thumbnails."),
            ("Waveforms", "waveforms",
             "Toggle the timeline audio waveforms."),
        ]
        labels = {
            "Minimize": "Minimize",
            "FrameView": "Frame View",
            "ScrollBars": "Scroll Bars",
            "AutoScroll": "Auto Scroll",
            "StopOnScrub": "Stop When Scrubbing",
            "TrackMedia": "Track Media",
            "Thumbnails": "Thumbnails",
            "Waveforms": "Waveforms",
        }
        for name, field, tooltip in items:
            action = ftk.Action(
                labels[name],
                checkedCallback = lambda checked, captured = field, \
                    f = Util.weak(self._setField): f(captured, checked))
            action.tooltip = tooltip
            self.actions[name] = action

        selfWeak = weakref.ref(self)
        self._settingsObserver = djv.models.TimelineSettingsObserver(
            app.getSettingsModel().observeTimeline,
            lambda value: selfWeak()._settingsUpdate(value))

    def _setField(self, field, value):
        model = self._app().getSettingsModel()
        settings = model.timeline
        setattr(settings, field, value)
        model.timeline = settings

    def _settingsUpdate(self, settings):
        self.actions["Minimize"].checked = settings.minimize
        self.actions["FrameView"].checked = settings.frameView
        self.actions["ScrollBars"].checked = settings.scrollBars
        self.actions["AutoScroll"].checked = settings.autoScroll
        self.actions["StopOnScrub"].checked = settings.stopOnScrub
        self.actions["TrackMedia"].checked = settings.trackMedia
        self.actions["Thumbnails"].checked = settings.thumbnails
        self.actions["Waveforms"].checked = settings.waveforms
