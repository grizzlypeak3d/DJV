# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the DJV project.

import opentimelineio as otio
import ftkPy as ftk
import tlRenderPy as tl
import djvPy as djv

import Tools

import weakref

class Actions:
    """
    This class provides the tool actions.
    """
    def __init__(self, context, app):

        self._app = weakref.ref(app)
        self.actions = {}
        self.tools = [
            tool for tool in app.getToolsModel().tools
            if tool.name in Tools.FACTORY]
        for tool in self.tools:
            action = ftk.Action(
                tool.name,
                tool.icon,
                tool.shortcut,
                checkedCallback = lambda checked, captured = tool.name:
                    self._toolCallback(captured, checked))
            action.tooltip = "Toggle the {} tool.".format(tool.name.lower())
            self.actions[tool.name] = action

        selfWeak = weakref.ref(self)
        self._openObserver = ftk.StringListObserver(
            app.getToolsModel().observeOpenTools,
            lambda names: selfWeak()._openUpdate(names))

    def _toolCallback(self, name, checked):
        self._app().getToolsModel().setToolOpen(name, checked)

    def _openUpdate(self, names):
        for tool in self.tools:
            self.actions[tool.name].checked = tool.name in names
