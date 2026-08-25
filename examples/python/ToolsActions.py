# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the DJV project.

import opentimelineio as otio
import ftkPy as ftk
import tlRenderPy as tl
import djvPy as djv

import IActions
import Tools
import Util

import weakref

class Actions(IActions.IActions):
    """
    This class provides the tool actions.
    """
    def __init__(self, context, app):
        IActions.IActions.__init__(self, context, app, "Tools")

        self._app = weakref.ref(app)
        self.tools = [
            tool for tool in app.getToolsModel().tools
            if tool.name in Tools.FACTORY]
        for tool in self.tools:
            self._addCheckCommand(
                tool.name,
                "Toggle the {} tool.".format(tool.name),
                lambda value, captured = tool.name, \
                    f = Util.weak(self._toolCallback): f(captured, value))
            self.actions[tool.name] = ftk.Action(
                tool.name,
                tool.icon,
                checkedCallback = self._checkCommand(tool.name))
            self._addShortcut(tool.name, tool.name, tool.shortcut)

        self._shortcutsUpdate(self._settingsModel.shortcuts)

        selfWeak = weakref.ref(self)
        self._openObserver = ftk.StringListObserver(
            app.getToolsModel().observeOpenTools,
            lambda names: selfWeak()._openUpdate(names))

    def _toolCallback(self, name, checked):
        app = self._app()
        app.getToolsModel().setToolOpen(name, checked)
        if checked:
            # Opening a tool while the panel is hidden would otherwise do
            # nothing that can be seen, which reads as the button being
            # broken.
            settingsModel = app.getSettingsModel()
            window = settingsModel.window
            if not window.tools:
                window.tools = True
                settingsModel.window = window

    def _openUpdate(self, names):
        for tool in self.tools:
            self.actions[tool.name].checked = tool.name in names
