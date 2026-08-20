# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the DJV project.

import opentimelineio as otio
import ftkPy as ftk
import tlRenderPy as tl
import djvPy as djv

import MainWindow

import weakref

class App(ftk.App):
    """
    The application creates the models and main window.

    The models come from the DJV libraries; this class is the application
    layer that the C++ App provides, written in Python: it watches the
    files model and turns the current file into a timeline player.
    """
    def __init__(self, context, argv):

        self._cmdLineInput = ftk.CmdLineArgString("Input", "Input file", True)

        ftk.App.__init__(
            self,
            context,
            argv,
            "djv-python",
            "DJV Python player",
            [ self._cmdLineInput ])

    def __del__(self):
        if hasattr(self, "_settingsModel"):
            self._settingsModel.save()

    def getSettings(self):
        return self._settings

    def getSettingsModel(self):
        return self._settingsModel

    def getFilesModel(self):
        return self._filesModel

    def getRecentFilesModel(self):
        return self._recentFilesModel

    def getTimeUnitsModel(self):
        return self._timeUnitsModel

    def getViewportModel(self):
        return self._viewportModel

    def getAudioModel(self):
        return self._audioModel

    def observePlayer(self):
        """
        Observe the player for the current file.
        """
        return self._player

    def open(self, path):
        """
        Open an image sequence, movie, or timeline file.
        """
        item = djv.models.FilesModelItem()
        item.path = path if isinstance(path, ftk.Path) else ftk.Path(str(path))
        self._filesModel.add(item)

    def run(self):

        # The settings file backs all of the models.
        docPath = ftk.getUserPath(ftk.UserPath.Documents)
        settingsPath = ftk.getSettingsPath(
            ftk.Path(docPath, "DJV").get(),
            "djv-python.json")
        self._settings = ftk.Settings(self.context, settingsPath)

        self._settingsModel = djv.models.SettingsModel(
            self.context, self._settings, 1.0)
        self._filesModel = djv.models.FilesModel(self._settings)
        self._recentFilesModel = djv.models.RecentFilesModel(
            self.context, self._settings)
        self._timeUnitsModel = djv.models.TimeUnitsModel(
            self.context, self._settings)
        self._viewportModel = djv.models.ViewportModel(
            self.context, self._settings)
        self._audioModel = djv.models.AudioModel(self.context, self._settings)

        self._player = tl.ObservablePlayer(None)
        self._aItem = None

        # Initialize the file browser.
        fileBrowserSystem = self.context.getSystemByName("ftk::FileBrowserSystem")
        fileBrowserSystem.model.exts = tl.getExts(self.context)
        fileBrowserSystem.recentFilesModel = self._recentFilesModel

        self._window = MainWindow.MainWindow(self.context, self)

        selfWeak = weakref.ref(self)
        self._aObserver = djv.models.FilesModelItemObserver(
            self._filesModel.observeA,
            lambda item: selfWeak()._aUpdate(item))
        self._volumeObserver = ftk.FloatObserver(
            self._audioModel.observeVolume,
            lambda value: selfWeak()._volumeUpdate(value))
        self._muteObserver = ftk.BoolObserver(
            self._audioModel.observeMute,
            lambda value: selfWeak()._muteUpdate(value))

        if self._cmdLineInput.hasValue:
            self.open(ftk.Path(self._cmdLineInput.value))

        super().run()

    def _aUpdate(self, item):

        # Playback carries over between files the way the C++ application
        # does it: the file being left remembers where it was.
        player = self._player.get()
        if self._aItem and player:
            self._aItem.currentTime = player.currentTime
            self._aItem.inOutRange = player.inOutRange
        self._aItem = item

        player = None
        if item:
            timeline = tl.Timeline(self.context, item.path)
            options = tl.PlayerOptions()
            options.cache = self._settingsModel.cache
            player = tl.Player(self.context, timeline, options)
            player.volume = self._audioModel.volume
            player.mute = self._audioModel.mute
            if item.currentTime is not None:
                player.currentTime = item.currentTime
            if item.inOutRange is not None:
                player.inOutRange = item.inOutRange
            if self._settingsModel.playback.startPlayback:
                player.forward()
            self._recentFilesModel.addRecent(item.path)
        self._player.setAlways(player)

    def _volumeUpdate(self, value):
        if self._player.get():
            self._player.get().volume = value

    def _muteUpdate(self, value):
        if self._player.get():
            self._player.get().mute = value
