# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the DJV project.

import opentimelineio as otio
import ftkPy as ftk
import tlRenderPy as tl
import djvPy as djv

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

        for name in ["B", "Wipe", "Overlay", "Difference"]:
            self.addAction(actions.actions[name])

class View(ftk.ToolBar):
    """
    View tool bar.
    """
    def __init__(self, context, actions, parent = None):
        ftk.ToolBar.__init__(self, context, ftk.Orientation.Horizontal, parent)

        self.addAction(actions.actions["Frame"])
        self.addAction(actions.actions["ZoomReset"])
        self.addAction(actions.actions["ZoomIn"])
        self.addAction(actions.actions["ZoomOut"])

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
