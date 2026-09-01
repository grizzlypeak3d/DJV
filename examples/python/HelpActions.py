# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the DJV project.

import opentimelineio as otio
import ftkPy as ftk
import tlRenderPy as tl
import djvPy as djv

import IActions
import Util

import weakref

class Actions(IActions.IActions):
    """
    This class provides the help actions.
    """
    def __init__(self, context, app, mainWindow):
        IActions.IActions.__init__(self, context, app, "Help")

        self._app = weakref.ref(app)
        self._mainWindowWeak = weakref.ref(mainWindow)
        self._sysInfoDialog = None

        # Register the commands.
        self._addCommand(
            "Documentation",
            "Open the documentation in a web browser.",
            lambda args, f = Util.weak(self._documentation): f())

        studioURL = app.getAppInfoModel().studioURL
        if studioURL:
            self._addCommand(
                "Studio",
                "Open the DJV Studio web site in a web browser.",
                lambda args: ftk.openURL(studioURL))

        self._addCommand(
            "About",
            "Show the about dialog.",
            lambda args, f = Util.weak(self._about): f())

        self._addCommand(
            "SysInfo",
            "Show the system information dialog.",
            lambda args, f = Util.weak(self._sysInfo): f())

        # Create the actions.
        self.actions["Documentation"] = ftk.Action(
            "Documentation",
            self._command("Documentation"))
        if not app.getAppInfoModel().docsURL:
            # A build that was not installed has none. Saying so is
            # better than a menu item that does nothing when it is
            # clicked.
            self.actions["Documentation"].enabled = False
            self.actions["Documentation"].tooltip = (
                "The documentation is installed with the application,\n"
                "and this build was not installed.")
        if studioURL:
            self.actions["Studio"] = ftk.Action(
                "DJV Studio",
                self._command("Studio"))
            self.actions["Studio"].tooltip = (
                "Open the web site for DJV Studio,\n"
                "the commercial version of DJV.")
        self.actions["About"] = ftk.Action(
            "About",
            self._command("About"))
        self.actions["SysInfo"] = ftk.Action(
            "System Information",
            self._command("SysInfo"))

        # Register the shortcuts.
        self._addShortcut("Documentation")
        self._addShortcut("About")
        self._addShortcut("SysInfo", "System Information")

        self._shortcutsUpdate(self._settingsModel.shortcuts)

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
        dialogSystem = self._app().context.getSystemByName("ftk::DialogSystem")
        dialogSystem.message("About", text, self._mainWindowWeak())

    def _sysInfo(self):
        app = self._app()
        mainWindow = self._mainWindowWeak()
        text = djv.ui.getSysInfo(
            app.context,
            app.getAppInfoModel(),
            app.getSettingsModel(),
            mainWindow.getWindowInfo())
        selfWeak = weakref.ref(self)
        self._sysInfoDialog = djv.ui.SysInfoDialog(app.context, text)
        self._sysInfoDialog.open(mainWindow)
        self._sysInfoDialog.setCloseCallback(
            lambda: selfWeak() and setattr(
                selfWeak(), "_sysInfoDialog", None))
