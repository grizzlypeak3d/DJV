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

class Window(ftk.Menu):
    """
    Window menu.
    """
    def __init__(self, context, app, actions, parent = None):
        ftk.Menu.__init__(self, context, parent)

        self.addAction(actions.actions["FullScreen"])
