# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the DJV project.

import opentimelineio as otio
import ftkPy as ftk
import tlRenderPy as tl
import djvPy as djv

import Util

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
        self.addAction(actions.actions["OpenAudio"])
        self.addAction(actions.actions["Close"])
        self.addAction(actions.actions["CloseAll"])
        self.addAction(actions.actions["Reload"])
        self.recentMenu = self.addSubMenu("Recent")
        self.addDivider();
        self.addAction(actions.actions["Next"])
        self.addAction(actions.actions["Prev"])
        self.addDivider();
        self.addAction(actions.actions["NextLayer"])
        self.addAction(actions.actions["PrevLayer"])
        self.addDivider();
        self.addAction(actions.actions["NextMediaReference"])
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
                lambda captured = recent, \
                    f = Util.weak(self._recentCallback): f(captured))
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
        self.addAction(actions.actions["Toggle"])
        self.addDivider();
        self.addAction(actions.actions["JumpBack1s"])
        self.addAction(actions.actions["JumpBack10s"])
        self.addAction(actions.actions["JumpForward1s"])
        self.addAction(actions.actions["JumpForward10s"])
        self.addDivider();
        self.addAction(actions.actions["Loop"])
        self.addAction(actions.actions["Once"])
        self.addAction(actions.actions["PingPong"])
        self.addDivider();
        self.addAction(actions.actions["SetInPoint"])
        self.addAction(actions.actions["ResetInPoint"])
        self.addAction(actions.actions["SetOutPoint"])
        self.addAction(actions.actions["ResetOutPoint"])

class Frame(ftk.Menu):
    """
    Frame menu.
    """
    def __init__(self, context, app, actions, parent = None):
        ftk.Menu.__init__(self, context, parent)

        self.addAction(actions.actions["Start"])
        self.addAction(actions.actions["End"])
        self.addDivider();
        self.addAction(actions.actions["Prev"])
        self.addAction(actions.actions["PrevX10"])
        self.addAction(actions.actions["PrevX100"])
        self.addDivider();
        self.addAction(actions.actions["Next"])
        self.addAction(actions.actions["NextX10"])
        self.addAction(actions.actions["NextX100"])
        self.addDivider();
        self.addAction(actions.actions["FocusCurrent"])

class Timeline(ftk.Menu):
    """
    Timeline menu.
    """
    def __init__(self, context, app, actions, parent = None):
        ftk.Menu.__init__(self, context, parent)

        self.addAction(actions.actions["Minimize"])
        self.addAction(actions.actions["FrameView"])
        self.addAction(actions.actions["ScrollBars"])
        self.addAction(actions.actions["AutoScroll"])
        self.addAction(actions.actions["StopOnScrub"])
        self.addDivider();
        self.addAction(actions.actions["TrackMedia"])
        self.addAction(actions.actions["Thumbnails"])
        self.addAction(actions.actions["Waveforms"])
        self.thumbnailSizeMenu = self.addSubMenu("Video Thumbnail Size")
        self.thumbnailSizeMenu.addAction(actions.actions["ThumbnailSizeSmall"])
        self.thumbnailSizeMenu.addAction(actions.actions["ThumbnailSizeMedium"])
        self.thumbnailSizeMenu.addAction(actions.actions["ThumbnailSizeLarge"])
        self.waveformSizeMenu = self.addSubMenu("Audio Waveform Size")
        self.waveformSizeMenu.addAction(actions.actions["WaveformSizeSmall"])
        self.waveformSizeMenu.addAction(actions.actions["WaveformSizeMedium"])
        self.waveformSizeMenu.addAction(actions.actions["WaveformSizeLarge"])

class Audio(ftk.Menu):
    """
    Audio menu.
    """
    def __init__(self, context, app, actions, parent = None):
        ftk.Menu.__init__(self, context, parent)

        self.addAction(actions.actions["VolumeUp"])
        self.addAction(actions.actions["VolumeDown"])
        self.addAction(actions.actions["Mute"])

class Color(ftk.Menu):
    """
    Color menu.
    """
    def __init__(self, context, app, actions, parent = None):
        ftk.Menu.__init__(self, context, parent)

        self.addAction(actions.actions["OCIO"])
        self.addAction(actions.actions["LUT"])

class Help(ftk.Menu):
    """
    Help menu.
    """
    def __init__(self, context, app, actions, parent = None):
        ftk.Menu.__init__(self, context, parent)

        self.addAction(actions.actions["Documentation"])
        if "Studio" in actions.actions:
            self.addAction(actions.actions["Studio"])
        self.addAction(actions.actions["About"])
        self.addAction(actions.actions["SysInfo"])

class Compare(ftk.Menu):
    """
    Compare menu.
    """
    def __init__(self, context, app, actions, parent = None):
        ftk.Menu.__init__(self, context, parent)

        self._app = weakref.ref(app)
        self._fileActions = []

        self.bMenu = self.addSubMenu("B")
        self.addAction(actions.actions["Next"])
        self.addAction(actions.actions["Prev"])
        self.addDivider();
        for mode in actions.modes:
            self.addAction(actions.actions[tl.to_string(mode)])
        self.addDivider();
        self.timeMenu = self.addSubMenu("Sync by")
        for time in tl.getCompareTimeEnums():
            self.timeMenu.addAction(actions.actions[tl.to_string(time)])

        selfWeak = weakref.ref(self)
        self._filesObserver = djv.models.FilesModelItemListObserver(
            app.getFilesModel().observeFiles,
            lambda files: selfWeak()._filesUpdate(files))
        self._bObserver = ftk.IntListObserver(
            app.getFilesModel().observeBIndexes,
            lambda indexes: selfWeak()._bUpdate(indexes))

    def _toggleB(self, index):
        if self._app():
            self._app().getFilesModel().toggleB(index)

    def _filesUpdate(self, files):
        self.bMenu.clear()
        self._fileActions = []
        for i, item in enumerate(files):
            action = ftk.Action(
                item.path.fileName,
                checkedCallback = lambda checked, captured = i, \
                    f = Util.weak(self._toggleB): f(captured))
            self._fileActions.append(action)
            self.bMenu.addAction(action)
        self._bUpdate(self._app().getFilesModel().bIndexes)

    def _bUpdate(self, indexes):
        for i, action in enumerate(self._fileActions):
            action.checked = i in indexes

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
        self.addAction(actions.actions["Center"])
        self.addDivider();
        self.addAction(actions.actions["Red"])
        self.addAction(actions.actions["Green"])
        self.addAction(actions.actions["Blue"])
        self.addAction(actions.actions["Alpha"])
        self.addDivider();
        self.addAction(actions.actions["Negative"])
        self.addDivider();
        self.addAction(actions.actions["MirrorHorizontal"])
        self.addAction(actions.actions["MirrorVertical"])
        self.addDivider();
        self.aspectRatioMenu = self.addSubMenu("Aspect Ratio")
        aspectRatioOptions = djv.models.AspectRatioOptions()
        for i in range(len(aspectRatioOptions.options)):
            self.aspectRatioMenu.addAction(
                actions.actions["AspectRatio_{}".format(i)])
        self.addDivider();
        self.addAction(actions.actions["Grid"])
        self.addAction(actions.actions["Outline"])
        self.addAction(actions.actions["CenterMarker"])
        self.addAction(actions.actions["HUD"])

class ToolsMenu(ftk.Menu):
    """
    Tools menu.
    """
    def __init__(self, context, app, actions, parent = None):
        ftk.Menu.__init__(self, context, parent)

        for tool in actions.tools:
            self.addAction(actions.actions[tool.name])

class Window(ftk.Menu):
    """
    Window menu.
    """
    def __init__(self, context, app, actions, parent = None):
        ftk.Menu.__init__(self, context, parent)

        self.addAction(actions.actions["FullScreen"])
        self.addAction(actions.actions["PresentMode"])
        self.addAction(actions.actions["FloatOnTop"])
        self.addDivider();
        self.addAction(actions.actions["FileToolBar"])
        self.addAction(actions.actions["CompareToolBar"])
        self.addAction(actions.actions["WindowToolBar"])
        self.addAction(actions.actions["ViewToolBar"])
        self.addAction(actions.actions["ToolsToolBar"])
        self.addAction(actions.actions["TabBar"])
        self.addAction(actions.actions["Timeline"])
        self.addAction(actions.actions["BottomToolBar"])
        self.addAction(actions.actions["StatusToolBar"])
        self.addAction(actions.actions["Tools"])
