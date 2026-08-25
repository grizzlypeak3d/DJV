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
    This class provides window actions.
    """
    def __init__(self, context, app, mainWindow):
        IActions.IActions.__init__(self, context, app, "Window")

        self._appWeak = weakref.ref(app)
        self._mainWindowWeak = weakref.ref(mainWindow)
        mainWindowWeak = self._mainWindowWeak

        # Register the commands.
        self._addCheckCommand(
            "FullScreen",
            "Toggle the window full screen.",
            lambda value: setattr(mainWindowWeak(), "fullScreen", value))

        self._addCheckCommand(
            "PresentMode",
            "Toggle presentation mode.",
            lambda value: mainWindowWeak().setPresentMode(value))

        self._addCheckCommand(
            "FloatOnTop",
            "Toggle the window floating on top.",
            lambda value: setattr(mainWindowWeak(), "floatOnTop", value))

        # The component toggles all write the window settings; the main
        # window observes the settings and shows or hides the widgets.
        components = [
            ("FileToolBar", "File Tool Bar", "fileToolBar", "Toggle the file tool bar."),
            ("CompareToolBar", "Compare Tool Bar", "compareToolBar", "Toggle the compare tool bar."),
            ("WindowToolBar", "Window Tool Bar", "windowToolBar", "Toggle the window tool bar."),
            ("ViewToolBar", "View Tool Bar", "viewToolBar", "Toggle the view tool bar."),
            ("ToolsToolBar", "Tools Tool Bar", "toolsToolBar", "Toggle the tools tool bar."),
            ("TabBar", "Tab Bar", "tabBar", "Toggle the tab bar."),
            ("Timeline", "Timeline", "timeline", "Toggle the timeline."),
            ("BottomToolBar", "Bottom Tool Bar", "bottomToolBar", "Toggle the bottom tool bar."),
            ("StatusToolBar", "Status Tool Bar", "statusToolBar", "Toggle the status tool bar."),
            ("Tools", "Tools", "tools", "Toggle the tools panel.")]
        self._components = components
        for name, text, field, tooltip in components:
            self._addCheckCommand(
                name,
                tooltip,
                lambda value, captured = field, \
                    f = Util.weak(self._componentCallback): f(captured, value))

        # Create the actions.
        self.actions["FullScreen"] = ftk.Action(
            "Full Screen",
            "WindowFullScreen",
            checkedCallback = self._checkCommand("FullScreen"))
        self.actions["PresentMode"] = ftk.Action(
            "Presentation",
            checkedCallback = self._checkCommand("PresentMode"))
        self.actions["FloatOnTop"] = ftk.Action(
            "Float On Top",
            checkedCallback = self._checkCommand("FloatOnTop"))
        for name, text, field, tooltip in components:
            self.actions[name] = ftk.Action(
                text,
                checkedCallback = self._checkCommand(name))

        # Register the shortcuts.
        self._addShortcut("FullScreen", ftk.Key.U)
        self._addShortcut("PresentMode", "Presentation Mode",
            ftk.KeyShortcut(ftk.Key.P, ftk.KeyModifier.Control))
        self._addShortcut("FloatOnTop")
        for name, text, field, tooltip in components:
            if "Tools" == name:
                self._addShortcut(name, "Tools Panel")
            else:
                self._addShortcut(name)

        self._shortcutsUpdate(self._settingsModel.shortcuts)

        selfWeak = weakref.ref(self)
        self._fullScreenObserver = ftk.BoolObserver(
            mainWindow.observeFullScreen,
            lambda value: setattr(
                selfWeak().actions["FullScreen"], "checked", value))
        self._presentModeObserver = ftk.BoolObserver(
            mainWindow.observePresentMode(),
            lambda value: setattr(
                selfWeak().actions["PresentMode"], "checked", value))
        self._floatOnTopObserver = ftk.BoolObserver(
            mainWindow.observeFloatOnTop,
            lambda value: setattr(
                selfWeak().actions["FloatOnTop"], "checked", value))
        self._settingsObserver = djv.models.WindowSettingsObserver(
            app.getSettingsModel().observeWindow,
            lambda value: selfWeak()._settingsUpdate(value))

    def _componentCallback(self, field, value):
        if self._appWeak():
            settingsModel = self._appWeak().getSettingsModel()
            settings = settingsModel.window
            setattr(settings, field, value)
            settingsModel.window = settings

    def _settingsUpdate(self, settings):
        for name, text, field, tooltip in self._components:
            self.actions[name].checked = getattr(settings, field)
