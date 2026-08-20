# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the DJV project.

import opentimelineio as otio
import ftkPy as ftk
import tlRenderPy as tl
import djvPy as djv

import weakref

class Widget(ftk.IContainer):
    """
    This widget provides a tab per open file.
    """
    def __init__(self, context, app, parent = None):
        ftk.IContainer.__init__(self, context, "TabBar.Widget", parent)

        self._app = weakref.ref(app)
        self._aIndex = -1

        self._tabBar = ftk.TabBar(context)
        self._tabBar.closable = True
        self._setWidget(self._tabBar)

        self._tabBar.setCallback(
            lambda index: self._app().getFilesModel().setA(index))
        self._tabBar.setTabCloseCallback(
            lambda index: self._app().getFilesModel().close(index))

        selfWeak = weakref.ref(self)
        self._filesObserver = djv.models.FilesModelItemListObserver(
            app.getFilesModel().observeFiles,
            lambda files: selfWeak()._filesUpdate(files))
        self._aIndexObserver = ftk.IntObserver(
            app.getFilesModel().observeAIndex,
            lambda value: selfWeak()._aIndexUpdate(value))

    def _filesUpdate(self, files):
        self._tabBar.clear()
        for item in files:
            self._tabBar.addTab(item.path.fileName, item.path.get())
        self._tabBar.current = self._aIndex

    def _aIndexUpdate(self, value):
        self._aIndex = value
        self._tabBar.current = value
