# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the DJV project.

import opentimelineio as otio
import ftkPy as ftk
import tlRenderPy as tl
import djvPy as djv

import weakref

class File(ftk.ToolBar):
    """
    File tool bar.
    """
    def __init__(self, context, actions, parent = None):
        ftk.ToolBar.__init__(self, context, ftk.Orientation.Horizontal, parent)

        self.addAction(actions.actions["Open"])
        self.addAction(actions.actions["Close"])
        self.addAction(actions.actions["CloseAll"])

class Compare(ftk.ToolBar):
    """
    Compare tool bar.
    """
    def __init__(self, context, actions, parent = None):
        ftk.ToolBar.__init__(self, context, ftk.Orientation.Horizontal, parent)

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

        self.addAction(actions.actions["Frame"])

        self._zoomEdit = ftk.DoubleEdit(context)
        self._zoomEdit.range = ftk.RangeD(0.0001, 1000.0)
        self._zoomEdit.step = 0.1
        self._zoomEdit.largeStep = 1.0
        self._zoomEdit.tooltip = "The view zoom."
        self.addWidget(self._zoomEdit)

        self.addAction(actions.actions["ZoomReset"])
        self.addAction(actions.actions["ZoomIn"])
        self.addAction(actions.actions["ZoomOut"])

        viewport = mainWindow.getViewport()
        viewportWeak = weakref.ref(viewport)
        self._zoomEdit.setCallback(
            lambda value: setattr(viewportWeak(), "zoom", value))
        selfWeak = weakref.ref(self)
        self._zoomObserver = ftk.DoubleObserver(
            viewport.observeZoom,
            lambda value: selfWeak()._zoomUpdate(value))

    def _zoomUpdate(self, value):
        self._zoomEdit.value = value

class Tools(ftk.ToolBar):
    """
    Tools tool bar.
    """
    def __init__(self, context, actions, parent = None):
        ftk.ToolBar.__init__(self, context, ftk.Orientation.Horizontal, parent)

        for tool in actions.tools:
            if tool.toolBar:
                self.addAction(actions.actions[tool.name])

class Window(ftk.ToolBar):
    """
    Window tool bar.
    """
    def __init__(self, context, actions, parent = None):
        ftk.ToolBar.__init__(self, context, ftk.Orientation.Horizontal, parent)

        self.addAction(actions.actions["FullScreen"])
