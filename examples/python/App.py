# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the DJV project.

import os
import weakref

import feather_tk as ftk
import djv

import NotesTool


def weak(method):
    """
    Wrap a bound method for use as a callback.

    A callback handed to the C++ side is held by a std::function, an edge
    the Python garbage collector cannot see, so a bound method or a lambda
    that captures self strongly forms a cycle that is never collected --
    and the settings are written when the objects are destroyed.
    """
    r = weakref.WeakMethod(method)

    def call(*args, **kwargs):
        m = r()
        if m is not None:
            return m(*args, **kwargs)
    return call


class AppInfoModel(djv.models.AppInfoModel):
    """
    The application says who it is: the name shows in the window title,
    the about dialog, and the system information. The documents directory
    stays DJV's, so the settings live beside the C++ application's.
    """
    def getFullName(self):
        return "DJV Python"

    def getShortName(self):
        return "djv-python"

    def getDocsDirName(self):
        return "DJV"

    def getDocsSearchPath(self):
        # The executable is the Python interpreter, which lives nowhere
        # near the install; examples/python sits beside the documentation
        # the way bin does.
        return os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


class App(djv.app.App):
    """
    The C++ application with two extensions: a tool, registered with the
    tools model and the tool widget factory once the application has
    created them, and a command with a menu item, added once the main
    window exists. The hooks are the application's own initialization
    steps, in the order they run: _modelsInit, _observersInit,
    _inputFilesInit, _uiInit, _mainWindowInit.
    """
    def __init__(self, context, argv):
        djv.app.App.__init__(self, context, argv, AppInfoModel())

    def _uiInit(self):
        djv.app.App._uiInit(self)
        info = djv.models.ToolInfo()
        info.name = NotesTool.NAME
        info.icon = NotesTool.ICON
        info.sort = NotesTool.NAME
        info.toolBar = True
        self.getToolsModel().addTool(info)
        self.getToolWidgetFactory().addTool(NotesTool.NAME, NotesTool.NotesTool)

    def _mainWindowInit(self):
        djv.app.App._mainWindowInit(self)
        # A command, which the command line and the capture tool can run
        # by name, and a menu item that runs it.
        self.getCommandsModel().add(
            "Notes/Clear",
            "Clear the notes.",
            weak(self._clearNotes))
        self._clearAction = ftk.Action("Clear Notes", weak(self._clearNotes))
        self.getMainWindow().getMenuBar().getMenu("Tools").addAction(self._clearAction)

    def _clearNotes(self, *args):
        tool = self.getMainWindow().getToolWidget(NotesTool.NAME)
        if tool is not None:
            tool.clear()
