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
    This class provides compare actions.
    """
    def __init__(self, context, app):
        IActions.IActions.__init__(self, context, app, "Compare")

        self._app = weakref.ref(app)
        appWeak = self._app

        # The compare modes, without "None": not comparing is the state
        # with none of them on, so it is not one of them and has no
        # action to tick.
        self.modes = [
            mode for mode in tl.getCompareEnums()
            if mode != tl.Compare._None]

        # Register the commands.
        self._addCommand(
            "Next",
            "Go to the next B file.",
            lambda args: appWeak().getFilesModel().nextB())

        self._addCommand(
            "Prev",
            "Go to the previous B file.",
            lambda args: appWeak().getFilesModel().prevB())

        docs = {
            "B": "Show the B file.",
            "Wipe": "Wipe between the A and B files.",
            "Butterfly": "Show the same half of the A and B files, one of "
                "them mirrored.",
            "Overlay": "Overlay the A and B files.",
            "Difference": "Show the difference between the A and B files.",
            "Horizontal": "Show the A and B files in a horizontal layout.",
            "Vertical": "Show the A and B files in a vertical layout.",
            "Tile": "Show the A and B files in a tiled layout.",
        }
        for mode in self.modes:
            name = tl.to_string(mode)
            self._addCheckCommand(
                name,
                docs.get(name, ""),
                lambda value, captured = mode, \
                    f = Util.weak(self._modeCallback): f(captured, value))

        self._addCommand(
            "Relative",
            "Compare files using relative time.",
            lambda args: setattr(
                appWeak().getFilesModel(), "compareTime",
                tl.CompareTime.Relative))

        self._addCommand(
            "Absolute",
            "Compare files using absolute time.",
            lambda args: setattr(
                appWeak().getFilesModel(), "compareTime",
                tl.CompareTime.Absolute))

        # Create the actions.
        self.actions["Next"] = ftk.Action(
            "Next",
            "Next",
            self._command("Next"))
        self.actions["Prev"] = ftk.Action(
            "Previous",
            "Prev",
            self._command("Prev"))
        for mode in self.modes:
            name = tl.to_string(mode)
            self.actions[name] = ftk.Action(
                tl.getLabel(mode),
                "Compare" + name,
                checkedCallback = self._checkCommand(name))
        # The keys and the commands keep the enumeration's names, which
        # are what the shortcuts are stored under; only what is shown
        # changes.
        compareTimeLabels = djv.models.getCompareTimeLabels()
        self.actions["Relative"] = ftk.Action(
            compareTimeLabels[0],
            self._command("Relative"))
        self.actions["Absolute"] = ftk.Action(
            compareTimeLabels[1],
            self._command("Absolute"))

        # Register the shortcuts.
        self._addShortcut("Next", ftk.KeyShortcut(ftk.Key.PageDown, ftk.KeyModifier.Shift))
        self._addShortcut("Prev", ftk.KeyShortcut(ftk.Key.PageUp, ftk.KeyModifier.Shift))
        self._addShortcut("B", ftk.KeyShortcut(ftk.Key.B, ftk.KeyModifier.Control))
        self._addShortcut("Wipe", ftk.KeyShortcut(ftk.Key.W, ftk.KeyModifier.Control))
        self._addShortcut("Butterfly")
        self._addShortcut("Overlay")
        self._addShortcut("Difference")
        self._addShortcut("Horizontal")
        self._addShortcut("Vertical")
        self._addShortcut("Tile", ftk.KeyShortcut(ftk.Key.T, ftk.KeyModifier.Control))
        self._addShortcut("Relative", compareTimeLabels[0])
        self._addShortcut("Absolute", compareTimeLabels[1])

        self._shortcutsUpdate(self._settingsModel.shortcuts)

        selfWeak = weakref.ref(self)
        self._compareObserver = djv.models.CompareOptionsObserver(
            app.getFilesModel().observeCompareOptions,
            lambda value: selfWeak()._compareUpdate(value))
        self._compareTimeObserver = djv.models.CompareTimeObserver(
            app.getFilesModel().observeCompareTime,
            lambda value: selfWeak()._compareTimeUpdate(value))
        self._filesObserver = djv.models.FilesModelItemListObserver(
            app.getFilesModel().observeFiles,
            lambda files: selfWeak()._filesUpdate(files))

    def _modeCallback(self, mode, value):
        filesModel = self._app().getFilesModel()
        options = filesModel.compareOptions
        options.compare = mode if value else tl.Compare._None
        filesModel.compareOptions = options

    def _compareUpdate(self, options):
        for mode in self.modes:
            self.actions[tl.to_string(mode)].checked = mode == options.compare

    def _compareTimeUpdate(self, time):
        self.actions["Relative"].checked = tl.CompareTime.Relative == time
        self.actions["Absolute"].checked = tl.CompareTime.Absolute == time

    def _filesUpdate(self, files):
        self.actions["Next"].enabled = len(files) > 1
        self.actions["Prev"].enabled = len(files) > 1
        for mode in self.modes:
            self.actions[tl.to_string(mode)].enabled = len(files) > 0
