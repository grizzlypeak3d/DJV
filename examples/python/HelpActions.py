# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the DJV project.

import opentimelineio as otio
import ftkPy as ftk
import tlRenderPy as tl
import djvPy as djv

import weakref

class Actions:
    """
    This class provides the help actions.
    """
    def __init__(self, context, app, mainWindow):

        self._context = context
        self._app = weakref.ref(app)
        self._mainWindowWeak = weakref.ref(mainWindow)
        self.actions = {}
        self.actions["Documentation"] = ftk.Action(
            "Documentation",
            self._documentation)
        self.actions["Documentation"].tooltip = \
            "Open the documentation in a web browser."

        self.actions["About"] = ftk.Action(
            "About",
            self._about)
        self.actions["About"].tooltip = "Show the about dialog."

    def _documentation(self):
        info = self._app().getAppInfoModel()
        ftk.openURL(info.docsURL)

    def _about(self):
        info = self._app().getAppInfoModel()
        text = "{} version: {}\nCommit date: {}\nCommit: {}".format(
            info.fullName,
            info.version,
            info.commitDate,
            info.gitCommit)
        dialogSystem = self._context.getSystemByName("ftk::DialogSystem")
        dialogSystem.message("About", text, self._mainWindowWeak())
