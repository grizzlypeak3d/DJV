# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the DJV project.

import weakref

import feather_tk as ftk
import tlrender as tl
import djv

NAME = "Notes"
ICON = "Info"


class NotesTool(djv.app.IToolWidget):
    """
    A tool panel written in Python: the frame, title and close button are
    the application's, and the content is a note for the current file. The
    pattern is the application's own: a model is observed, and the widget
    updates itself from the observer rather than from its own callback.
    """
    def __init__(self, context, app, mainWindow, parent=None):
        djv.app.IToolWidget.__init__(
            self, context, app, mainWindow, NAME, ICON, "NotesTool", parent)

        self._fileLabel = ftk.Label(context)
        self._edit = ftk.LineEdit(context)
        ftk.setScreenshotTag(self._edit, "Notes.Edit")

        layout = ftk.FormLayout(context)
        layout.marginRole = ftk.SizeRole.Margin
        layout.spacingRole = ftk.SizeRole.SpacingSmall
        layout.addRow("File:", self._fileLabel)
        layout.addRow("Note:", self._edit)
        self.setWidget(layout)

        selfWeak = weakref.ref(self)
        self._playerObserver = tl.PlayerObserver(
            app.observePlayer,
            lambda value: selfWeak()._playerUpdate(value))

    def clear(self):
        self._edit.text = ""

    def _playerUpdate(self, player):
        self._fileLabel.text = player.path.get() if player is not None else ""
