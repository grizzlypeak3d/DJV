# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the DJV project.

import opentimelineio as otio
import ftkPy as ftk
import tlRenderPy as tl
import djvPy as djv

import os
import weakref

class File(ftk.Menu):
    """
    File menu.
    """
    def __init__(self, context, app, actions, parent = None):
        ftk.Menu.__init__(self, context, parent)

        self._app = weakref.ref(app)

        self.addAction(actions.actions["Open"])
        self.addAction(actions.actions["Close"])
        self.addAction(actions.actions["CloseAll"])
        self.addAction(actions.actions["Reload"])
        self.recentMenu = self.addSubMenu("Recent")
        self.addDivider();
        self.addAction(actions.actions["Next"])
        self.addAction(actions.actions["Prev"])
        self.addDivider();
        self.addAction(actions.actions["Exit"])

        selfWeak = weakref.ref(self)
        self.recentObserver = ftk.PathListObserver(
            app.getRecentFilesModel().observeRecent,
            lambda recentList: selfWeak()._recentUpdate(recentList))

    def _recentCallback(self, recent):
        if (self._app):
            self._app().open(recent)

    def _recentUpdate(self, recentList):
        self.recentMenu.clear()
        for recent in reversed(recentList):
            action = ftk.Action(
                recent.fileName,
                lambda captured = recent: self._recentCallback(captured))
            self.recentMenu.addAction(action)

class Playback(ftk.Menu):
    """
    Playback menu.
    """
    def __init__(self, context, app, actions, parent = None):
        ftk.Menu.__init__(self, context, parent)

        self.addAction(actions.actions["Stop"])
        self.addAction(actions.actions["Forward"])
        self.addAction(actions.actions["Reverse"])
        self.addAction(actions.actions["TogglePlayback"])
        self.addDivider();
        self.addAction(actions.actions["Start"])
        self.addAction(actions.actions["Prev"])
        self.addAction(actions.actions["Next"])
        self.addAction(actions.actions["End"])
        self.addDivider();
        self.addAction(actions.actions["SetInPoint"])
        self.addAction(actions.actions["ResetInPoint"])
        self.addAction(actions.actions["SetOutPoint"])
        self.addAction(actions.actions["ResetOutPoint"])

class Compare(ftk.Menu):
    """
    Compare menu.
    """
    def __init__(self, context, app, actions, parent = None):
        ftk.Menu.__init__(self, context, parent)

        self._app = weakref.ref(app)
        self._fileActions = []

        self.bMenu = self.addSubMenu("B")
        self.addAction(actions.actions["Next"])
        self.addAction(actions.actions["Prev"])
        self.addDivider();
        for mode in actions.modes:
            self.addAction(actions.actions[tl.to_string(mode)])
        self.addDivider();
        self.timeMenu = self.addSubMenu("Sync by")
        for time in tl.getCompareTimeEnums():
            self.timeMenu.addAction(actions.actions[tl.to_string(time)])

        selfWeak = weakref.ref(self)
        self._filesObserver = djv.models.FilesModelItemListObserver(
            app.getFilesModel().observeFiles,
            lambda files: selfWeak()._filesUpdate(files))
        self._bObserver = ftk.IntListObserver(
            app.getFilesModel().observeBIndexes,
            lambda indexes: selfWeak()._bUpdate(indexes))

    def _toggleB(self, index):
        if self._app():
            self._app().getFilesModel().toggleB(index)

    def _filesUpdate(self, files):
        self.bMenu.clear()
        self._fileActions = []
        for i, item in enumerate(files):
            action = ftk.Action(
                item.path.fileName,
                checkedCallback = lambda checked, captured = i:
                    self._toggleB(captured))
            self._fileActions.append(action)
            self.bMenu.addAction(action)
        self._bUpdate(self._app().getFilesModel().bIndexes)

    def _bUpdate(self, indexes):
        for i, action in enumerate(self._fileActions):
            action.checked = i in indexes

class View(ftk.Menu):
    """
    View menu.
    """
    def __init__(self, context, app, actions, parent = None):
        ftk.Menu.__init__(self, context, parent)

        self.addAction(actions.actions["Frame"])
        self.addAction(actions.actions["ZoomReset"])
        self.addAction(actions.actions["ZoomIn"])
        self.addAction(actions.actions["ZoomOut"])
        self.addDivider();
        self.addAction(actions.actions["HUD"])

class ToolsMenu(ftk.Menu):
    """
    Tools menu.
    """
    def __init__(self, context, app, actions, parent = None):
        ftk.Menu.__init__(self, context, parent)

        for tool in actions.tools:
            self.addAction(actions.actions[tool.name])

class Window(ftk.Menu):
    """
    Window menu.
    """
    def __init__(self, context, app, actions, parent = None):
        ftk.Menu.__init__(self, context, parent)

        self.addAction(actions.actions["FullScreen"])
