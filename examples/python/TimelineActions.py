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
    This class provides the timeline actions.
    """
    def __init__(self, context, app):
        IActions.IActions.__init__(self, context, app, "Timeline")

        self._app = weakref.ref(app)

        # Register the commands.
        checks = [
            ("Minimize", "Minimize", "minimize", "Minimize the timeline."),
            ("FrameView", "Frame View", "frameView", "Frame the timeline view."),
            ("ScrollBars", "Scroll Bars", "scrollBars", "Toggle the scroll bars."),
            ("AutoScroll", "Auto Scroll", "autoScroll",
             "Automatically scroll the timeline to the current frame."),
            ("StopOnScrub", "Stop When Scrubbing", "stopOnScrub",
             "Stop playback when scrubbing the timeline."),
            ("TrackMedia", "Track Media", "trackMedia",
             "Toggle the timeline video thumbnails and audio waveforms."),
            ("Thumbnails", "Thumbnails", "thumbnails",
             "Toggle the timeline video thumbnails."),
            ("Waveforms", "Waveforms", "waveforms",
             "Toggle the timeline audio waveforms."),
        ]
        for name, text, field, doc in checks:
            self._addCheckCommand(
                name,
                doc,
                lambda value, captured = field, \
                    f = Util.weak(self._setField): f(captured, value))

        sizes = [
            ("ThumbnailSizeSmall", "Small", "thumbnailSize",
             djv.models.TimelineThumbnailSize.Small, "Small timeline thumbnails."),
            ("ThumbnailSizeMedium", "Medium", "thumbnailSize",
             djv.models.TimelineThumbnailSize.Medium, "Medium timeline thumbnails."),
            ("ThumbnailSizeLarge", "Large", "thumbnailSize",
             djv.models.TimelineThumbnailSize.Large, "Large timeline thumbnails."),
            ("WaveformSizeSmall", "Small", "waveformSize",
             djv.models.TimelineThumbnailSize.Small, "Small timeline audio waveforms."),
            ("WaveformSizeMedium", "Medium", "waveformSize",
             djv.models.TimelineThumbnailSize.Medium, "Medium timeline audio waveforms."),
            ("WaveformSizeLarge", "Large", "waveformSize",
             djv.models.TimelineThumbnailSize.Large, "Large timeline audio waveforms."),
        ]
        for name, text, field, size, doc in sizes:
            self._addCommand(
                name,
                doc,
                lambda args, capturedField = field, capturedSize = size, \
                    f = Util.weak(self._setField): f(capturedField, capturedSize))

        # Create the actions.
        for name, text, field, doc in checks:
            self.actions[name] = ftk.Action(
                text,
                checkedCallback = self._checkCommand(name))
        for name, text, field, size, doc in sizes:
            self.actions[name] = ftk.Action(
                text,
                self._command(name))

        # Register the shortcuts.
        self._addShortcut("Minimize")
        self._addShortcut("FrameView")
        self._addShortcut("ScrollBars")
        self._addShortcut("AutoScroll")
        self._addShortcut("StopOnScrub", "Stop On Scrub")
        self._addShortcut("Thumbnails")
        self._addShortcut("ThumbnailSizeSmall", "Small Video Thumbnails")
        self._addShortcut("ThumbnailSizeMedium", "Medium Video Thumbnails")
        self._addShortcut("ThumbnailSizeLarge", "Large Video Thumbnails")
        self._addShortcut("Waveforms")
        self._addShortcut("TrackMedia", ftk.Key.C)
        self._addShortcut("WaveformSizeSmall", "Small Audio Waveforms")
        self._addShortcut("WaveformSizeMedium", "Medium Audio Waveforms")
        self._addShortcut("WaveformSizeLarge", "Large Audio Waveforms")

        self._shortcutsUpdate(self._settingsModel.shortcuts)

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
        Small = djv.models.TimelineThumbnailSize.Small
        Medium = djv.models.TimelineThumbnailSize.Medium
        Large = djv.models.TimelineThumbnailSize.Large
        self.actions["ThumbnailSizeSmall"].checked = Small == settings.thumbnailSize
        self.actions["ThumbnailSizeMedium"].checked = Medium == settings.thumbnailSize
        self.actions["ThumbnailSizeLarge"].checked = Large == settings.thumbnailSize
        self.actions["WaveformSizeSmall"].checked = Small == settings.waveformSize
        self.actions["WaveformSizeMedium"].checked = Medium == settings.waveformSize
        self.actions["WaveformSizeLarge"].checked = Large == settings.waveformSize
