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
        self._cmdLineB = ftk.CmdLineOptionString(
            ["-b"], "Open a file for comparison.")
        self._cmdLineCompare = ftk.CmdLineOptionString(
            ["-compare"], "The comparison mode.")

        ftk.App.__init__(
            self,
            context,
            argv,
            "djv-python",
            "DJV Python player",
            [ self._cmdLineInput ],
            [ self._cmdLineB, self._cmdLineCompare ])

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

    def getColorModel(self):
        return self._colorModel

    def observePlayer(self):
        """
        Observe the player for the current file.
        """
        return self._player

    def open(self, path):
        """
        Open an image sequence, movie, or timeline file.
        """
        path = path if isinstance(path, ftk.Path) else ftk.Path(str(path))
        options = ftk.DirListOptions()
        options.seqExts = tl.getExts(self.context, int(tl.FileType.Seq))
        options.seqMaxDigits = self._settingsModel.imageSeq.maxDigits
        options.seq = True
        for i in tl.getPaths(self.context, path, options):
            item = djv.models.FilesModelItem()
            item.path = i
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
        self._colorModel = djv.models.ColorModel(self.context, self._settings)

        self._player = tl.ObservablePlayer(None)
        self._files = []
        self._timelines = []
        self._activeFiles = []

        # Initialize the file browser.
        fileBrowserSystem = self.context.getSystemByName("ftk::FileBrowserSystem")
        fileBrowserSystem.model.exts = tl.getExts(self.context)
        fileBrowserSystem.recentFilesModel = self._recentFilesModel

        self._window = MainWindow.MainWindow(self.context, self)

        selfWeak = weakref.ref(self)
        self._filesObserver = djv.models.FilesModelItemListObserver(
            self._filesModel.observeFiles,
            lambda files: selfWeak()._filesUpdate(files))
        self._activeObserver = djv.models.FilesModelItemListObserver(
            self._filesModel.observeActive,
            lambda files: selfWeak()._activeUpdate(files))
        self._compareTimeObserver = djv.models.CompareTimeObserver(
            self._filesModel.observeCompareTime,
            lambda value: selfWeak()._compareTimeUpdate(value))
        self._volumeObserver = ftk.FloatObserver(
            self._audioModel.observeVolume,
            lambda value: selfWeak()._volumeUpdate(value))
        self._muteObserver = ftk.BoolObserver(
            self._audioModel.observeMute,
            lambda value: selfWeak()._muteUpdate(value))

        if self._cmdLineInput.hasValue:
            self.open(ftk.Path(self._cmdLineInput.value))
        if self._cmdLineB.hasValue:
            self.open(ftk.Path(self._cmdLineB.value))
            self._filesModel.setB(len(self._filesModel.files) - 1, True)
            self._filesModel.setA(0)
        if self._cmdLineCompare.hasValue:
            for mode in tl.getCompareEnums():
                if tl.getLabel(mode).lower() == self._cmdLineCompare.value.lower():
                    options = self._filesModel.compareOptions
                    options.compare = mode
                    self._filesModel.compareOptions = options

        super().run()

    def _createTimeline(self, item):
        options = tl.Options()
        imageSeq = self._settingsModel.imageSeq
        options.imageSeqAudio = imageSeq.audio
        options.imageSeqAudioExts = imageSeq.audioExts
        options.imageSeqAudioFileName = imageSeq.audioFileName
        options.readThreadCount = imageSeq.readThreadCount
        options.compat = self._settingsModel.otio.compat
        options.ioOptions = self._settingsModel.ioOptions
        options.pathOptions.seqMaxDigits = imageSeq.maxDigits
        return tl.Timeline(self.context, item.path, item.audioPath, options)

    def _filesUpdate(self, files):

        # Timelines follow the files, reused for the files that stay.
        timelines = []
        for item in files:
            timeline = None
            for i, existing in enumerate(self._files):
                if existing is item:
                    timeline = self._timelines[i]
            if timeline is None:
                timeline = self._createTimeline(item)
                self._recentFilesModel.addRecent(item.path)
            timelines.append(timeline)
        self._files = list(files)
        self._timelines = timelines

    def _activeUpdate(self, activeFiles):

        # Playback carries over between files the way the C++ application
        # does it: the file being left remembers where it was.
        player = self._player.get()
        if self._activeFiles and player:
            self._activeFiles[0].currentTime = player.currentTime
            self._activeFiles[0].inOutRange = player.inOutRange
        prevA = self._activeFiles[0] if self._activeFiles else None
        self._activeFiles = list(activeFiles)

        if not activeFiles:
            self._player.setAlways(None)
            return

        item = activeFiles[0]
        if item is not prevA or not player:
            timeline = self._timelineForItem(item)
            playerOptions = tl.PlayerOptions()
            playerOptions.cache = self._settingsModel.cache
            player = tl.Player(self.context, timeline, playerOptions)
            player.volume = self._audioModel.volume
            player.mute = self._audioModel.mute
            if item.currentTime is not None:
                player.currentTime = item.currentTime
            if item.inOutRange is not None:
                player.inOutRange = item.inOutRange
            if self._settingsModel.playback.startPlayback and item.newFile:
                player.forward()
            item.newFile = False
            self._player.setAlways(player)

        player = self._player.get()
        player.compare = [self._timelineForItem(i) for i in activeFiles[1:]]
        player.compareTime = self._filesModel.compareTime

    def _timelineForItem(self, item):
        for i, existing in enumerate(self._files):
            if existing is item:
                return self._timelines[i]
        timeline = self._createTimeline(item)
        self._files.append(item)
        self._timelines.append(timeline)
        return timeline

    def _compareTimeUpdate(self, value):
        if self._player.get():
            self._player.get().compareTime = value

    def _volumeUpdate(self, value):
        if self._player.get():
            self._player.get().volume = value

    def _muteUpdate(self, value):
        if self._player.get():
            self._player.get().mute = value
