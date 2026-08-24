# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the DJV project.

import opentimelineio as otio
import ftkPy as ftk
import tlRenderPy as tl
import djvPy as djv

import Util

import weakref

class Actions:
    """
    This class provides window actions.
    """
    def __init__(self, context, app, mainWindow):

        self._appWeak = weakref.ref(app)
        self._mainWindowWeak = weakref.ref(mainWindow)
        self.actions = {}
        self.actions["FullScreen"] = ftk.Action(
            "Full Screen",
            "WindowFullScreen",
            ftk.KeyShortcut(ftk.Key.U),
            checkedCallback = Util.weak(self._fullScreenCallback))
        self.actions["FullScreen"].tooltip = "Toggle the window full screen."

        self.actions["PresentMode"] = ftk.Action(
            "Presentation",
            ftk.KeyShortcut(ftk.Key.P, ftk.KeyModifier.Control),
            checkedCallback = Util.weak(self._presentModeCallback))
        self.actions["PresentMode"].tooltip = "Toggle presentation mode."

        self.actions["FloatOnTop"] = ftk.Action(
            "Float On Top",
            checkedCallback = Util.weak(self._floatOnTopCallback))
        self.actions["FloatOnTop"].tooltip = "Toggle the window floating on top."

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
            self.actions[name] = ftk.Action(
                text,
                checkedCallback = lambda value, captured = field, \
                    f = Util.weak(self._componentCallback): f(captured, value))
            self.actions[name].tooltip = tooltip

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

    def _fullScreenCallback(self, value):
        if self._mainWindowWeak():
            self._mainWindowWeak().fullScreen = value

    def _presentModeCallback(self, value):
        if self._mainWindowWeak():
            self._mainWindowWeak().setPresentMode(value)

    def _floatOnTopCallback(self, value):
        if self._mainWindowWeak():
            self._mainWindowWeak().floatOnTop = value

    def _componentCallback(self, field, value):
        if self._appWeak():
            settingsModel = self._appWeak().getSettingsModel()
            settings = settingsModel.window
            setattr(settings, field, value)
            settingsModel.window = settings

    def _settingsUpdate(self, settings):
        for name, text, field, tooltip in self._components:
            self.actions[name].checked = getattr(settings, field)
