# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the DJV project.

import opentimelineio as otio
import ftkPy as ftk
import tlRenderPy as tl
import djvPy as djv

import Util

import weakref

class File(ftk.ToolBar):
    """
    File tool bar.
    """
    def __init__(self, context, actions, parent = None):
        ftk.ToolBar.__init__(self, context, ftk.Orientation.Horizontal, parent)

        self.marginRole = ftk.SizeRole.MarginInside
        self.addAction(actions.actions["Open"])
        self.addAction(actions.actions["OpenAudio"])
        self.addAction(actions.actions["Close"])
        self.addAction(actions.actions["CloseAll"])
        self.addAction(actions.actions["Reload"])

class Compare(ftk.ToolBar):
    """
    Compare tool bar.
    """
    def __init__(self, context, actions, parent = None):
        ftk.ToolBar.__init__(self, context, ftk.Orientation.Horizontal, parent)

        self.marginRole = ftk.SizeRole.MarginInside
        for mode in actions.modes:
            if tl.Compare._None == mode:
                continue
            self.addAction(actions.actions[tl.to_string(mode)])

class View(ftk.ToolBar):
    """
    View tool bar.
    """
    def __init__(self, context, actions, mainWindow, parent = None):
        ftk.ToolBar.__init__(self, context, ftk.Orientation.Horizontal, parent)

        self._updating = False

        self.marginRole = ftk.SizeRole.MarginInside
        self.addAction(actions.actions["Frame"])

        viewport = mainWindow.getViewport()
        self._zoomEdit = ftk.DoubleEdit(context)
        self._zoomEdit.range = viewport.zoomRange
        self._zoomEdit.step = 0.1
        self._zoomEdit.largeStep = 1.0
        self._zoomEdit.defaultValue = 1.0
        self._zoomEdit.precision = 2
        self._zoomEdit.borderRole = ftk.ColorRole._None
        self._zoomEdit.tooltip = "View zoom"
        self.addWidget(self._zoomEdit)

        self.addAction(actions.actions["ZoomReset"])

        viewportWeak = weakref.ref(viewport)
        self._zoomEdit.setCallback(Util.weak(self._zoomCallback))
        self._viewportWeak = viewportWeak
        selfWeak = weakref.ref(self)
        self._posZoomObserver = tl.ui.ViewPosAndZoomObserver(
            viewport.observeViewPosAndZoom,
            lambda value: selfWeak()._zoomUpdate(value[1]))
        # There is no zoom to show without an image; the actions beside
        # this are gated the same way in ViewActions.
        self._playerObserver = tl.PlayerObserver(
            mainWindow.app.observePlayer(),
            lambda player: setattr(
                selfWeak()._zoomEdit, "enabled",
                player is not None and len(player.ioInfo.video) > 0))

    def _zoomCallback(self, value):
        if not self._updating:
            viewport = self._viewportWeak()
            if viewport is not None:
                g = viewport.geometry
                focus = ftk.V2I(
                    (g.max.x - g.min.x + 1) // 2,
                    (g.max.y - g.min.y + 1) // 2)
                viewport.setZoom(value, focus)

    def _zoomUpdate(self, value):
        self._updating = True
        self._zoomEdit.value = value
        self._updating = False

class Tools(ftk.ToolBar):
    """
    Tools tool bar.
    """
    def __init__(self, context, actions, parent = None):
        ftk.ToolBar.__init__(self, context, ftk.Orientation.Horizontal, parent)

        self.marginRole = ftk.SizeRole.MarginInside
        for tool in actions.tools:
            if tool.toolBar:
                self.addAction(actions.actions[tool.name])

class Window(ftk.ToolBar):
    """
    Window tool bar.
    """
    def __init__(self, context, actions, parent = None):
        ftk.ToolBar.__init__(self, context, ftk.Orientation.Horizontal, parent)

        self.marginRole = ftk.SizeRole.MarginInside
        self.addAction(actions.actions["FullScreen"])
