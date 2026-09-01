# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the DJV project.

import opentimelineio as otio
import ftkPy as ftk
import tlRenderPy as tl
import djvPy as djv

import AudioActions
import ColorActions
import CompareActions
import FileActions
import FrameActions
import HelpActions
import Menus
import PlaybackActions
import ReviewActions
import TimelineActions
import PlaybackBar
import StatusBar
import TabBar
import ToolBars
import Tools
import ToolsActions
import Util
import ViewActions
import WindowActions

import os
import weakref

class MainWindow(ftk.MainWindow):
    """
    The main window creates the widgets and actions.
    """
    def __init__(self, context, app):

        self._app = weakref.ref(app)
        self._settingsModel = app.getSettingsModel()
        window = self._settingsModel.window
        ftk.MainWindow.__init__(self, context, app, window.size)

        # Created before the actions; the window actions observe it.
        self._presentMode = ftk.ObservableBool(False)

        # Every icon compiled into the resource library, the application
        # icon included.
        djv.ui.initIcons(context)
        iconSystem = context.getSystemByName("ftk::IconSystem")
        self.setIcon(iconSystem.get("DJV_Icon", 1.0))

        self.title = app.getAppInfoModel().title

        # Create the viewport and timeline, driven by the DJV models.
        self._viewport = djv.ui.Viewport(
            context,
            app.getFilesModel(),
            app.getColorModel(),
            app.getViewportModel(),
            app.getTimeUnitsModel(),
            app.getSettingsModel(),
            app.getAnnotationsModel(),
            app.getDrawModel(),
            app.getSysLogModel())
        self._timelineWidget = tl.ui.TimelineWidget(context, app.getTimeUnitsModel())
        # The current file's timeline and no other; left unset the widget
        # also draws one for each file being compared.
        self._timelineWidget.setTimelines([])

        # Create the actions.
        self._fileActions = FileActions.Actions(context, app, self)
        self._reviewActions = ReviewActions.Actions(context, app, self)
        self._compareActions = CompareActions.Actions(context, app)
        self._playbackActions = PlaybackActions.Actions(context, app)
        self._frameActions = FrameActions.Actions(context, app, self)
        self._timelineActions = TimelineActions.Actions(context, app)
        self._audioActions = AudioActions.Actions(context, app)
        self._viewActions = ViewActions.Actions(context, app, self)
        self._colorActions = ColorActions.Actions(context, app)
        self._toolsActions = ToolsActions.Actions(context, app)
        self._windowActions = WindowActions.Actions(context, app, self)
        self._helpActions = HelpActions.Actions(context, app, self)

        # Create the menu bar. The menus are kept in a dictionary as well
        # as added to the menu bar: the C++ side only keeps the C++ half
        # of a menu alive, and a Python menu that loses its Python half
        # loses its observers and callbacks with it.
        self._menus = {}
        self._menus["File"] = Menus.File(context, app, self._fileActions)
        self._menus["Review"] = Menus.Review(context, app, self._reviewActions)
        self._menus["Compare"] = Menus.Compare(context, app, self._compareActions)
        self._menus["Playback"] = Menus.Playback(context, app, self._playbackActions)
        self._menus["Frame"] = Menus.Frame(context, app, self._frameActions)
        self._menus["Timeline"] = Menus.Timeline(context, app, self._timelineActions)
        self._menus["Audio"] = Menus.Audio(context, app, self._audioActions)
        self._menus["View"] = Menus.View(context, app, self._viewActions)
        self._menus["Window"] = Menus.Window(context, app, self._windowActions)
        self._menus["Color"] = Menus.Color(context, app, self._colorActions)
        self._menus["Tools"] = Menus.ToolsMenu(context, app, self._toolsActions)
        self._menus["Help"] = Menus.Help(context, app, self._helpActions)
        self._menuBar = ftk.MenuBar(context)
        for name in self._menus:
            self._menuBar.addMenu(name, self._menus[name])
        self.menuBar = self._menuBar

        # Create the tool bars.
        self._fileToolBar = ToolBars.File(context, self._fileActions)
        self._compareToolBar = ToolBars.Compare(context, self._compareActions)
        self._viewToolBar = ToolBars.View(context, self._viewActions, self)
        self._toolsToolBar = ToolBars.Tools(context, self._toolsActions)
        self._windowToolBar = ToolBars.Window(context, self._windowActions)
        self._playbackBar = PlaybackBar.Widget(
            context, app, self._playbackActions, self._frameActions,
            self._reviewActions)
        self._tabBar = TabBar.Widget(context, app)
        self._statusBar = StatusBar.Widget(context, app, self)

        # Layout widgets. The dividers are kept by name so that hiding a
        # tool bar can hide its divider with it.
        self._dividers = {}
        self._layout = ftk.VerticalLayout(context)
        self._layout.spacingRole = ftk.SizeRole._None
        self.widget = self._layout
        hLayout = ftk.HorizontalLayout(context, self._layout)
        hLayout.spacingRole = ftk.SizeRole.Spacing
        self._toolBarLayout = hLayout
        self._fileToolBar.parent = hLayout
        self._dividers["File"] = ftk.Divider(
            context, ftk.Orientation.Horizontal, hLayout)
        self._compareToolBar.parent = hLayout
        self._dividers["Compare"] = ftk.Divider(
            context, ftk.Orientation.Horizontal, hLayout)
        self._windowToolBar.parent = hLayout
        self._dividers["Window"] = ftk.Divider(
            context, ftk.Orientation.Horizontal, hLayout)
        self._viewToolBar.parent = hLayout
        self._dividers["View"] = ftk.Divider(
            context, ftk.Orientation.Horizontal, hLayout)
        self._toolsToolBar.parent = hLayout
        self._dividers["ToolBars"] = ftk.Divider(
            context, ftk.Orientation.Vertical, self._layout)
        self._tabBar.parent = self._layout
        self._splitter = ftk.Splitter(context, ftk.Orientation.Vertical, self._layout)
        self._splitter.split = window.splitter
        self._splitter2 = ftk.Splitter(context, ftk.Orientation.Horizontal, self._splitter)
        self._splitter2.split = window.splitter2
        self._viewport.parent = self._splitter2
        self._toolsWidget = Tools.ToolsWidget(context, app, self)
        self._toolsWidget.parent = self._splitter2
        self._timelineWidget.parent = self._splitter
        self._dividers["Bottom"] = ftk.Divider(
            context, ftk.Orientation.Vertical, self._layout)
        self._playbackBar.parent = self._layout
        self._dividers["Status"] = ftk.Divider(
            context, ftk.Orientation.Vertical, self._layout)
        self._statusBar.parent = self._layout

        # Each context menu offers the band it belongs to, and only that
        # band; the Window menu remains the one place that lists every
        # piece of chrome together.
        hLayout.setContextMenuCallback(
            lambda f = Util.weak(self._chromeMenu): f([
                "FileToolBar",
                "CompareToolBar",
                "WindowToolBar",
                "ViewToolBar",
                "ToolsToolBar"]))

        # The timeline, playback controls and status bar form one band
        # across the bottom of the window, so right clicking any of them
        # offers the whole band rather than only itself.
        bottomChrome = ["Timeline", "BottomToolBar", "StatusToolBar"]
        self._timelineWidget.setContextMenuCallback(
            lambda f = Util.weak(self._chromeMenu): f(bottomChrome))
        self._playbackBar.setContextMenuCallback(
            lambda f = Util.weak(self._chromeMenu): f(bottomChrome))
        self._statusBar.setContextMenuCallback(
            lambda f = Util.weak(self._chromeMenu): f(bottomChrome))

        # Anywhere the click is not claimed, offer every toggle. The
        # window itself is consulted last, so a band keeps its own menu;
        # and because the viewport can never be hidden, this is the one
        # door that cannot be closed by hiding chrome.
        self.setContextMenuCallback(Util.weak(self._windowChromeMenu))

        # Create observers.
        selfWeak = weakref.ref(self)
        self._playerObserver = tl.PlayerObserver(
            app.observePlayer(),
            lambda player: selfWeak()._playerUpdate(player))
        appWeak = weakref.ref(app)
        self._compareObserver = djv.models.CompareOptionsObserver(
            self._viewport.observeCompareOptions,
            lambda value: setattr(
                appWeak().getFilesModel(), "compareOptions", value))
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
        self.saveSettings()

    def saveSettings(self):
        window = self._settingsModel.window
        window.size = self.size
        window.splitter = self._splitter.split
        window.splitter2 = self._splitter2.split
        self._settingsModel.window = window

    def getTimelineWidget(self):
        return self._timelineWidget

    def getViewport(self):
        return self._viewport

    def observePresentMode(self):
        return self._presentMode

    def focusCurrentFrame(self):
        self._playbackBar.focusCurrentFrame()

    def focusReviewNote(self):
        app = self._app()
        if app is None:
            return
        # Opening the tool creates it if this is the first time.
        app.getToolsModel().setToolOpen("Review", True)
        widget = self._toolsWidget.getToolWidget("Review")
        if widget is not None:
            widget.focusNote()

    def setPresentMode(self, value):
        if self._presentMode.setIfChanged(value):
            self.fullScreen = value
            self._windowUpdate()

    def dropEvent(self, event):
        event.accept = True
        if isinstance(event.data, ftk.DragDropTextData):
            if event.data.text:
                self.app.open(event.data.text[0])

    def keyPressEvent(self, event):
        if 0 == event.modifiers and \
                ftk.Key.Escape == event.key and \
                self._presentMode.get():
            event.accept = True
            self.setPresentMode(False)
        else:
            # The menu bar is asked directly so that the shortcuts keep
            # working while presentation mode hides it.
            event.accept = self._menuBar.shortcut(event.key, event.modifiers)

    def keyReleaseEvent(self, event):
        event.accept = True

    def _chromeMenu(self, names):
        # A context menu of window chrome visibility toggles. The actions
        # are the Window menu's own, so the check marks stay in sync and
        # the toggles go through the same commands; the Window menu
        # remains the complete inventory, and these are a second door to
        # part of it.
        menu = ftk.Menu(self.context)
        for name in names:
            menu.addAction(self._windowActions.actions[name])
        return menu

    def _windowChromeMenu(self):
        # Presentation mode hides every piece of chrome regardless of
        # these settings, so the toggles would do nothing you could see.
        # Escape leaves the mode.
        if self._presentMode.get():
            return None
        return self._chromeMenu([
            "FileToolBar",
            "CompareToolBar",
            "WindowToolBar",
            "ViewToolBar",
            "ToolsToolBar",
            "TabBar",
            "Timeline",
            "BottomToolBar",
            "StatusToolBar"])

    def _playerUpdate(self, player):
        self._viewport.player = player
        self._timelineWidget.player = player

    def _windowSettingsUpdate(self, settings):
        self._windowUpdate()

    def _windowUpdate(self):
        settings = self._settingsModel.window
        presentMode = self._presentMode.get()

        # The menu bar is removed rather than hidden: the base class owns
        # the divider under it, and setting the menu bar is what hides
        # both.
        if presentMode:
            self.menuBar = None
        elif self.menuBar is None:
            self.menuBar = self._menuBar

        self._fileToolBar.setVisible(settings.fileToolBar and not presentMode)
        self._dividers["File"].setVisible(settings.fileToolBar and not presentMode)

        self._compareToolBar.setVisible(settings.compareToolBar and not presentMode)
        self._dividers["Compare"].setVisible(settings.compareToolBar and not presentMode)

        self._windowToolBar.setVisible(settings.windowToolBar and not presentMode)
        self._dividers["Window"].setVisible(settings.windowToolBar and not presentMode)

        self._viewToolBar.setVisible(settings.viewToolBar and not presentMode)
        self._dividers["View"].setVisible(settings.viewToolBar and not presentMode)

        self._toolsToolBar.setVisible(settings.toolsToolBar and not presentMode)

        self._dividers["ToolBars"].setVisible(
            (settings.fileToolBar or
                settings.compareToolBar or
                settings.windowToolBar or
                settings.viewToolBar or
                settings.toolsToolBar) and not presentMode)

        self._tabBar.setVisible(settings.tabBar and not presentMode)

        self._toolsWidget.setDisplayed(settings.tools and not presentMode)

        self._timelineWidget.setVisible(settings.timeline and not presentMode)

        self._playbackBar.setVisible(settings.bottomToolBar and not presentMode)
        self._dividers["Bottom"].setVisible(settings.bottomToolBar and not presentMode)

        self._statusBar.setVisible(settings.statusToolBar and not presentMode)
        self._dividers["Status"].setVisible(settings.statusToolBar and not presentMode)

        # Not in presentation mode: an error balloon over someone else's
        # review is worse than a missed message, and the messages tool
        # still has them. The HUD is hidden rather than turned off, so
        # that what was being shown is still being shown on the way back
        # out.
        self._viewport.setToastActive(not presentMode)
        self._viewport.setHUDActive(not presentMode)

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
        display = self._timelineWidget.displayOptions
        display.ocio = value
        self._timelineWidget.displayOptions = display

    def _lutUpdate(self, value):
        display = self._timelineWidget.displayOptions
        display.lut = value
        self._timelineWidget.displayOptions = display
