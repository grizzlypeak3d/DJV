# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the DJV project.

import opentimelineio as otio
import ftkPy as ftk
import tlRenderPy as tl
import djvPy as djv

import weakref

class Actions:
    """
    This class provides compare actions.
    """
    def __init__(self, context, app):

        self._app = weakref.ref(app)
        self.actions = {}

        # One radio action per compare mode; "None" shows the "A" icon
        # since no comparison means the "A" file alone.
        self.modes = tl.getCompareEnums()
        self.modeGroup = ftk.ActionGroup(ftk.ActionGroupType.Radio)
        for mode in self.modes:
            name = tl.to_string(mode)
            icon = "CompareA" if tl.Compare._None == mode else "Compare" + name
            action = ftk.Action(
                tl.getLabel(mode),
                icon,
                checkedCallback = lambda checked, captured = mode:
                    self._modeCallback(captured) if checked else None)
            self.actions[name] = action
            self.modeGroup.addAction(action)

        self.actions["Next"] = ftk.Action(
            "Next B File",
            lambda: self._app().getFilesModel().nextB())
        self.actions["Next"].tooltip = "Change to the next \"B\" file."

        self.actions["Prev"] = ftk.Action(
            "Previous B File",
            lambda: self._app().getFilesModel().prevB())
        self.actions["Prev"].tooltip = "Change to the previous \"B\" file."

        # The time sync modes.
        self.timeGroup = ftk.ActionGroup(ftk.ActionGroupType.Radio)
        for time in tl.getCompareTimeEnums():
            action = ftk.Action(
                tl.getLabel(time),
                checkedCallback = lambda checked, captured = time:
                    self._timeCallback(captured) if checked else None)
            self.actions[tl.to_string(time)] = action
            self.timeGroup.addAction(action)

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

    def _modeCallback(self, mode):
        filesModel = self._app().getFilesModel()
        options = filesModel.compareOptions
        options.compare = mode
        filesModel.compareOptions = options

    def _compareUpdate(self, options):
        self.modeGroup.checked = self.modes.index(options.compare)

    def _timeCallback(self, time):
        self._app().getFilesModel().compareTime = time

    def _compareTimeUpdate(self, time):
        self.timeGroup.checked = tl.getCompareTimeEnums().index(time)

    def _filesUpdate(self, files):
        enabled = len(files) > 1
        self.actions["Next"].enabled = enabled
        self.actions["Prev"].enabled = enabled
