# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the DJV project.

import unittest

import djv


class AppTest(unittest.TestCase):
    """The application bindings are there; starting the application needs
    OpenGL, which the example and its capture test cover."""

    def test_classes(self):
        self.assertTrue(issubclass(djv.app.App, object))
        self.assertTrue(hasattr(djv.app.App, "_uiInit"))
        self.assertTrue(hasattr(djv.app.App, "_mainWindowInit"))
        self.assertTrue(hasattr(djv.app.IToolWidget, "setWidget"))
        self.assertTrue(hasattr(djv.app.ToolWidgetFactory, "addTool"))
        self.assertTrue(hasattr(djv.app.MainWindow, "getMenuBar"))
