# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the DJV project.

import opentimelineio as otio
import ftkPy as ftk
import tlRenderPy as tl
import djvPy as djv

import CompareActions
import FileActions
import Menus
import PlaybackActions
import PlaybackBar
import StatusBar
import TabBar
import ToolBars
import ViewActions
import WindowActions

import weakref

class MainWindow(ftk.MainWindow):
    """
    The main window creates the widgets and actions.
    """
    def __init__(self, context, app):

        self._settingsModel = app.getSettingsModel()
        window = self._settingsModel.window
        ftk.MainWindow.__init__(self, context, app, window.size)

        # Create the viewport and timeline, driven by the DJV models.
        self._viewport = tl.ui.Viewport(context)
        self._timelineWidget = tl.ui.TimelineWidget(context, app.getTimeUnitsModel())
        # The current file's timeline and no other; left unset the widget
        # also draws one for each file being compared.
        self._timelineWidget.setTimelines([])

        # Create the actions.
        self._fileActions = FileActions.Actions(context, app, self)
        self._compareActions = CompareActions.Actions(context, app)
        self._playbackActions = PlaybackActions.Actions(context, app)
        self._viewActions = ViewActions.Actions(context, app, self)
        self._windowActions = WindowActions.Actions(context, app, self)

        # Create the menu bar.
        self._menuBar = ftk.MenuBar(context)
        self._menuBar.addMenu("File", Menus.File(context, app, self._fileActions))
        self._menuBar.addMenu("Compare", Menus.Compare(context, app, self._compareActions))
        self._menuBar.addMenu("Playback", Menus.Playback(context, app, self._playbackActions))
        self._menuBar.addMenu("View", Menus.View(context, app, self._viewActions))
        self._menuBar.addMenu("Window", Menus.Window(context, app, self._windowActions))
        self.menuBar = self._menuBar

        # Create the tool bars.
        self._fileToolBar = ToolBars.File(context, self._fileActions)
        self._compareToolBar = ToolBars.Compare(context, self._compareActions)
        self._viewToolBar = ToolBars.View(context, self._viewActions)
        self._windowToolBar = ToolBars.Window(context, self._windowActions)
        self._playbackBar = PlaybackBar.Widget(context, app, self._playbackActions)
        self._tabBar = TabBar.Widget(context, app)
        self._tabBar.setVisible(window.tabBar)
        self._statusBar = StatusBar.Widget(context, app, self)

        # Layout widgets.
        self._layout = ftk.VerticalLayout(context)
        self._layout.spacingRole = ftk.SizeRole._None
        self.widget = self._layout
        hLayout = ftk.HorizontalLayout(context, self._layout)
        hLayout.spacingRole = ftk.SizeRole.SpacingSmall
        self._fileToolBar.parent = hLayout
        ftk.Divider(context, ftk.Orientation.Horizontal, hLayout)
        self._compareToolBar.parent = hLayout
        ftk.Divider(context, ftk.Orientation.Horizontal, hLayout)
        self._viewToolBar.parent = hLayout
        ftk.Divider(context, ftk.Orientation.Horizontal, hLayout)
        self._windowToolBar.parent = hLayout
        ftk.Divider(context, ftk.Orientation.Vertical, self._layout)
        self._tabBar.parent = self._layout
        self._splitter = ftk.Splitter(context, ftk.Orientation.Vertical, self._layout)
        self._splitter.split = window.splitter
        self._viewport.parent = self._splitter
        vLayout = ftk.VerticalLayout(context, self._splitter)
        vLayout.spacingRole = ftk.SizeRole._None
        self._playbackBar.parent = vLayout
        ftk.Divider(context, ftk.Orientation.Vertical, vLayout)
        self._timelineWidget.parent = vLayout
        ftk.Divider(context, ftk.Orientation.Vertical, self._layout)
        self._statusBar.parent = self._layout

        # Create observers.
        selfWeak = weakref.ref(self)
        self._playerObserver = tl.PlayerObserver(
            app.observePlayer(),
            lambda player: selfWeak()._playerUpdate(player))
        viewportModel = app.getViewportModel()
        self._bgObserver = djv.models.BackgroundOptionsObserver(
            viewportModel.observeBackgroundOptions,
            lambda value: selfWeak()._bgUpdate(value))
        self._fgObserver = djv.models.ForegroundOptionsObserver(
            viewportModel.observeForegroundOptions,
            lambda value: selfWeak()._fgUpdate(value))
        self._compareObserver = djv.models.CompareOptionsObserver(
            app.getFilesModel().observeCompareOptions,
            lambda value: selfWeak()._compareUpdate(value))
        self._windowSettingsObserver = djv.models.WindowSettingsObserver(
            self._settingsModel.observeWindow,
            lambda value: selfWeak()._windowSettingsUpdate(value))
        self._timelineSettingsObserver = djv.models.TimelineSettingsObserver(
            self._settingsModel.observeTimeline,
            lambda value: selfWeak()._timelineSettingsUpdate(value))
        self._timelineFrameViewObserver = ftk.BoolObserver(
            self._timelineWidget.observeFrameView,
            lambda value: selfWeak()._timelineFrameViewUpdate(value))
        colorModel = app.getColorModel()
        self._ocioObserver = djv.models.OCIOOptionsObserver(
            colorModel.observeResolvedOCIOOptions,
            lambda value: selfWeak()._ocioUpdate(value))
        self._lutObserver = djv.models.LUTOptionsObserver(
            colorModel.observeLUTOptions,
            lambda value: selfWeak()._lutUpdate(value))

    def __del__(self):
        window = self._settingsModel.window
        window.size = self.size
        window.splitter = self._splitter.split
        self._settingsModel.window = window

    def getViewport(self):
        return self._viewport

    def dropEvent(self, event):
        event.accept = True
        if isinstance(event.data, ftk.DragDropTextData):
            if event.data.text:
                self.app.open(event.data.text[0])

    def _playerUpdate(self, player):
        self._viewport.player = player
        self._timelineWidget.player = player

    def _bgUpdate(self, value):
        self._viewport.backgroundOptions = value

    def _fgUpdate(self, value):
        self._viewport.foregroundOptions = value

    def _compareUpdate(self, value):
        self._viewport.compareOptions = value

    def _windowSettingsUpdate(self, settings):
        self._tabBar.setVisible(settings.tabBar)

    def _timelineSettingsUpdate(self, settings):
        self._timelineWidget.frameView = settings.frameView
        self._timelineWidget.scrollBarsVisible = settings.scrollBars
        self._timelineWidget.autoScroll = settings.autoScroll
        self._timelineWidget.stopOnScrub = settings.stopOnScrub
        display = self._timelineWidget.displayOptions
        display.minimize = settings.minimize
        # Track media gates the two rather than replacing them, so that
        # turning it off and on leaves the choice below it alone.
        display.thumbnails = settings.trackMedia and settings.thumbnails
        display.thumbnailHeight = djv.models.getTimelineThumbnailSize(settings.thumbnailSize)
        display.waveforms = settings.trackMedia and settings.waveforms
        display.waveformHeight = djv.models.getTimelineWaveformSize(settings.waveformSize)
        self._timelineWidget.displayOptions = display

    def _timelineFrameViewUpdate(self, value):
        settings = self._settingsModel.timeline
        if settings.frameView != value:
            settings.frameView = value
            self._settingsModel.timeline = settings

    def _ocioUpdate(self, value):
        self._viewport.ocioOptions = value
        display = self._timelineWidget.displayOptions
        display.ocio = value
        self._timelineWidget.displayOptions = display

    def _lutUpdate(self, value):
        self._viewport.LUTOptions = value
        display = self._timelineWidget.displayOptions
        display.lut = value
        self._timelineWidget.displayOptions = display
