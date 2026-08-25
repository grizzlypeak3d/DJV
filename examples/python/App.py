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
        self._cmdLineListCommands = ftk.CmdLineFlag(
            ["-listCommands"], "Print the list of commands and exit.")
        self._cmdLineCommand = ftk.CmdLineListOptionString(
            ["-command"],
            "Execute a command after startup. The command name may be "
            "followed by JSON arguments; e.g., \"Playback/Forward\" or "
            "\"Playback/Seek { \\\"frame\\\": 100 }\". This option may be "
            "repeated to execute multiple commands in order. Use "
            "-listCommands to see the available commands.",
            "Commands")

        # The base class keeps the settings and log files under the same
        # directory as the C++ application, with this application's own
        # base name so the two do not read each other's settings. This
        # also adds the -settingsFile, -logFile, and -resetSettings
        # command line options.
        self._appInfoModel = djv.models.AppInfoModel()
        ftk.App.__init__(
            self,
            context,
            argv,
            "djv-python",
            "DJV Python player",
            [ self._cmdLineInput ],
            [ self._cmdLineB, self._cmdLineCompare,
              self._cmdLineListCommands, self._cmdLineCommand ],
            ftk.AppFiles(
                self._appInfoModel.docsDirName,
                "djv-python",
                self._appInfoModel.versionMajor))

    def __del__(self):
        if hasattr(self, "_settingsModel"):
            self._settingsModel.save()

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

    def getToolsModel(self):
        return self._toolsModel

    def getSysLogModel(self):
        return self._sysLogModel

    def getCommandsModel(self):
        return self._commandsModel

    def getAppInfoModel(self):
        return self._appInfoModel

    def observePlayer(self):
        """
        Observe the player for the current file.
        """
        return self._player

    def open(self, path, audioPath = None):
        """
        Open an image sequence, movie, or timeline file, optionally with
        a separate audio file.
        """
        path = path if isinstance(path, ftk.Path) else ftk.Path(str(path))
        options = ftk.DirListOptions()
        options.seqExts = tl.getExts(self.context, int(tl.FileType.Seq))
        options.seqMaxDigits = self._settingsModel.imageSeq.maxDigits
        options.seq = True
        for i in tl.getPaths(self.context, path, options):
            item = djv.models.FilesModelItem()
            item.path = i
            if audioPath is not None:
                item.audioPath = audioPath
            self._filesModel.add(item)

    def reload(self):
        """
        Reload the current file: the active files' timelines are dropped
        and rebuilt, so the file is read from disk again.
        """
        activeFiles = list(self._activeFiles)
        files = list(self._files)
        for item in activeFiles:
            for i, existing in enumerate(self._files):
                if existing is item:
                    del self._files[i]
                    del self._timelines[i]
                    break
        self._activeFiles = []
        if activeFiles:
            player = self._player.get()
            if player:
                activeFiles[0].speed = player.speed
                activeFiles[0].currentTime = player.currentTime
                activeFiles[0].inOutRange = player.inOutRange
        thumbnailSystem = self.context.getSystemByName(
            "tl::ui::ThumbnailSystem")
        thumbnailSystem.clearCache()
        self._filesUpdate(files)
        self._activeUpdate(activeFiles)
        # The items are the same objects holding different things now,
        # and the list of them did not change, so say so.
        self._filesModel.refresh()

    def openSeparateAudioDialog(self):
        """
        Open the dialog for choosing a file with a separate audio file.
        """
        selfWeak = weakref.ref(self)
        self._separateAudioDialog = djv.ui.SeparateAudioDialog(self.context)
        self._separateAudioDialog.open(self._window)
        def callback(path, audioPath):
            if selfWeak():
                selfWeak().open(path, audioPath)
                selfWeak()._separateAudioDialog.close()
        self._separateAudioDialog.setCallback(callback)
        self._separateAudioDialog.setCloseCallback(
            lambda: selfWeak() and setattr(
                selfWeak(), "_separateAudioDialog", None))

    def run(self):

        # The settings file from the base class backs all of the models.
        self._settings = self.settings

        self._settingsModel = djv.models.SettingsModel(
            self.context, self._settings, self.defaultDisplayScale)
        self._filesModel = djv.models.FilesModel(self._settings)
        self._recentFilesModel = djv.models.RecentFilesModel(
            self.context, self._settings)
        self._timeUnitsModel = djv.models.TimeUnitsModel(
            self.context, self._settings)
        self._viewportModel = djv.models.ViewportModel(
            self.context, self._settings)
        self._audioModel = djv.models.AudioModel(self.context, self._settings)
        self._colorModel = djv.models.ColorModel(self.context, self._settings)
        self._toolsModel = djv.models.ToolsModel(self._settings)
        self._sysLogModel = ftk.SysLogModel(self.context)
        self._commandsModel = djv.models.CommandsModel(self.context)

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
        self._styleSettingsObserver = djv.models.StyleSettingsObserver(
            self._settingsModel.observeStyle,
            lambda value: selfWeak()._styleUpdate(value))

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

        if self._cmdLineListCommands.found:
            for command in self._commandsModel.commands:
                print("{} - {}".format(command.name, command.doc))
            return

        if self._cmdLineCommand.list:
            # Wait for the command line inputs to be opened before
            # executing the commands.
            selfWeak = weakref.ref(self)
            self._commandTicks = 0
            self._commandTimer = ftk.Timer(self.context)
            self._commandTimer.repeating = True
            self._commandTimer.start(
                0.1, lambda: selfWeak()._commandTimeout())

        super().run()
        self._saveSettings()

    def _saveSettings(self):
        """
        Everything is written at the clean quit; the writes in the
        destructors are a backstop. A leak that keeps a model alive no
        longer loses its settings.
        """
        self._window.saveSettings()
        self._timeUnitsModel.save()
        self._filesModel.save()
        self._recentFilesModel.save()
        self._viewportModel.save()
        self._colorModel.save()
        self._audioModel.save()
        self._toolsModel.save()
        self._settingsModel.save()
        self.settings.save()

    def _commandTimeout(self):
        self._commandTicks += 1
        if not self._cmdLineInput.hasValue or \
                self._player.get() or \
                self._commandTicks > 100:
            self._commandTimer.stop()
            for value in self._cmdLineCommand.list:
                # Split the command name from the optional JSON arguments
                # at the first '{', so that command names may contain
                # spaces (e.g., "Tools/Color Picker").
                i = value.find("{")
                if i >= 0:
                    name = value[:i].rstrip()
                    args = value[i:]
                else:
                    name = value.rstrip()
                    args = "null"
                try:
                    self._commandsModel.exec(name, args)
                except RuntimeError as e:
                    print("Cannot parse command arguments: {}".format(e))

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
            self._activeFiles[0].speed = player.speed
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
            if item.speed >= 0.0:
                player.speed = item.speed
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

    def _styleUpdate(self, value):
        fontSystem = self.fontSystem
        fonts = fontSystem.fonts
        for font in value.fontFiles:
            if font:
                path = ftk.Path(font)
                fontName = path.base + path.num
                if fontName not in fonts:
                    fontSystem.addFont(fontName, font)
        style = self.style
        style.colorControls = value.colorControls
        style.fonts = value.fonts
        self.colorStyle = value.colorStyle
        self.customColorRoles = value.customColorRoles
        self.displayScale = value.displayScale
