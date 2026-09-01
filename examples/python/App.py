# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the DJV project.

import opentimelineio as otio
import ftkPy as ftk
import tlRenderPy as tl
import djvPy as djv

import MainWindow

import os
import weakref

class AppInfoModel(djv.models.AppInfoModel):
    """
    The application says who it is: the name shows in the window title,
    the about dialog, and the system information. The documents
    directory stays DJV's, so the settings live beside the C++
    application's rather than scattering.
    """
    def getFullName(self):
        return "DJV Python"

    def getShortName(self):
        return "djv-python"

    def getDocsDirName(self):
        # The base defaults this to the full name, which would move the
        # settings; they stay in DJV's directory.
        return "DJV"

    def getDocsSearchPath(self):
        # The default is the executable's directory, but this
        # application's executable is the Python interpreter, which
        # lives nowhere near the install. The application's own
        # directory serves instead: examples/python sits beside the
        # documentation the same way bin does.
        return os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

class App(ftk.App):
    """
    The application creates the models and main window.

    The models come from the DJV libraries; this class is the application
    layer that the C++ App provides, written in Python: it watches the
    files model and turns the current file into a timeline player.
    """
    def __init__(self, context, argv):

        self._cmdLineInputs = ftk.CmdLineListArgString(
            "input",
            "One or more timelines, movies, image sequences, or directories.",
            True)
        self._cmdLineAudio = ftk.CmdLineOptionString(
            ["-audio", "-a"], "Audio file name.", "Audio")
        self._cmdLineCompare = ftk.CmdLineOptionString(
            ["-compare", "-b"], "Compare \"B\" file name.", "Compare")
        self._cmdLineCompareMode = ftk.CmdLineOptionString(
            ["-compareMode", "-c"], "Compare mode.", "Compare")
        self._cmdLineWipeCenter = ftk.CmdLineOptionString(
            ["-wipeCenter", "-wc"],
            "Wipe center; e.g., \"0.5,0.5\".", "Compare")
        self._cmdLineWipeRotation = ftk.CmdLineOptionF(
            ["-wipeRotation", "-wr"], "Wipe rotation.", "Compare")
        self._cmdLineFrameRange = ftk.CmdLineOptionString(
            ["-frameRange", "-fr"],
            "Frame range of an image sequence (e.g., 1-100). This is the "
            "range the sequence is meant to cover, which need not be the "
            "frames on disk. Applies to the first file opened.",
            "Playback")
        self._cmdLineSpeed = ftk.CmdLineOptionD(
            ["-speed"], "Playback speed.", "Playback")
        self._cmdLinePlayback = ftk.CmdLineOptionString(
            ["-playback", "-p"], "Playback mode.", "Playback")
        self._cmdLineLoop = ftk.CmdLineOptionString(
            ["-loop"], "Loop mode.", "Playback")
        self._cmdLineTimeUnits = ftk.CmdLineOptionString(
            ["-timeUnits", "-tu"], "Set the time units.", "Playback")
        self._cmdLineSeek = ftk.CmdLineOptionString(
            ["-seek"], "Seek to the given time.", "Playback")
        self._cmdLineInPoint = ftk.CmdLineOptionString(
            ["-inPoint", "-in"], "Set the in point.", "Playback")
        self._cmdLineOutPoint = ftk.CmdLineOptionString(
            ["-outPoint", "-out"], "Set the out point.", "Playback")
        self._cmdLineDirFilter = ftk.CmdLineOptionString(
            ["-dirFilter"],
            "Filter the files when opening a directory: a "
            "case-insensitive substring, or a wildcard pattern with "
            "\"*\" and \"?\" (e.g., \"*.mov\").",
            "Directories")
        self._cmdLineDirDepth = ftk.CmdLineOptionI(
            ["-dirDepth"],
            "How many directory levels to open: 1 opens the directory "
            "alone.",
            "Directories",
            1)
        self._cmdLineOCIO = ftk.CmdLineOptionString(
            ["-ocio"],
            "OCIO configuration file name (e.g., config.ocio).", "Color")
        self._cmdLineOCIOInput = ftk.CmdLineOptionString(
            ["-ocioInput"], "OCIO input name.", "Color")
        self._cmdLineOCIODisplay = ftk.CmdLineOptionString(
            ["-ocioDisplay"], "OCIO display name.", "Color")
        self._cmdLineOCIOView = ftk.CmdLineOptionString(
            ["-ocioView"], "OCIO view name.", "Color")
        self._cmdLineOCIOLook = ftk.CmdLineOptionString(
            ["-ocioLook"], "OCIO look name.", "Color")
        self._cmdLineLUT = ftk.CmdLineOptionString(
            ["-lut"], "LUT file name.", "Color")
        self._cmdLineLUTOrder = ftk.CmdLineOptionString(
            ["-lutOrder"], "LUT operation order.", "Color")
        self._cmdLineSysInfo = ftk.CmdLineFlag(
            ["-sysInfo"], "Print the system information and exit.")
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
        self._appInfoModel = AppInfoModel()
        ftk.App.__init__(
            self,
            context,
            argv,
            "djv-python",
            "DJV Python player",
            [ self._cmdLineInputs ],
            [ self._cmdLineAudio, self._cmdLineCompare,
              self._cmdLineCompareMode, self._cmdLineWipeCenter,
              self._cmdLineWipeRotation, self._cmdLineSpeed,
              self._cmdLinePlayback, self._cmdLineLoop,
              self._cmdLineTimeUnits, self._cmdLineSeek,
              self._cmdLineFrameRange, self._cmdLineInPoint,
              self._cmdLineOutPoint, self._cmdLineDirFilter,
              self._cmdLineDirDepth, self._cmdLineOCIO,
              self._cmdLineOCIOInput, self._cmdLineOCIODisplay,
              self._cmdLineOCIOView, self._cmdLineOCIOLook,
              self._cmdLineLUT, self._cmdLineLUTOrder,
              self._cmdLineSysInfo,
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

    def getAnnotationsModel(self):
        return self._annotationsModel

    def getDrawModel(self):
        return self._drawModel

    def getNotesModel(self):
        return self._notesModel

    def getRangesModel(self):
        return self._rangesModel

    def getRecentReviewsModel(self):
        return self._recentReviewsModel

    def getRecentPlaylistsModel(self):
        return self._recentPlaylistsModel

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

    def open(self, path, audioPath = None, frames = None):
        """
        Open an image sequence, movie, or timeline file, optionally with
        a separate audio file and a stated frame range.
        """
        path = path if isinstance(path, ftk.Path) else ftk.Path(str(path))
        options = ftk.DirListOptions()
        options.seqExts = tl.getExts(self.context, int(tl.FileType.Seq))
        options.seqMaxDigits = self._settingsModel.imageSeq.maxDigits
        # Gathering a directory's frames into sequences and taking one
        # frame to name its sequence are the same thing said twice; a
        # stated range has already said what it wants.
        options.seq = frames is None
        # The command line said how directories are read; that holds
        # for the whole session, dialogs included.
        if self._cmdLineDirFilter.hasValue:
            options.filter = self._cmdLineDirFilter.value
        if self._cmdLineDirDepth.found:
            options.depth = max(1, self._cmdLineDirDepth.value)
        first = True
        for i in tl.getPaths(self.context, path, options):
            item = djv.models.FilesModelItem()
            # Annotations reference their source by this identity, so it
            # has to exist from the moment the file is opened.
            item.id = djv.models.generateId()
            item.path = i
            if first and frames is not None:
                # Stated, so the frames on disk are not looked for and
                # the range is used as it is. A directory gives several
                # sequences and one range cannot describe them all, so
                # only the first takes it.
                item.path.frames = frames
                item.framesStated = True
            first = False
            if audioPath is not None:
                item.audioPath = audioPath
            self._filesModel.add(item)

    def openPlaylist(self, path):
        """
        Open a playlist into the file list. Opening a ".otio" file
        normally plays it as a timeline in one tab; this expands it
        instead. The file cannot say which is meant, so the caller does.
        """
        path = path if isinstance(path, ftk.Path) else ftk.Path(str(path))
        playlist, report = djv.models.playlistOpen(path.getFileName(True))
        offset = len(self._filesModel.files)
        self._filesModel.add(playlist.items)
        if playlist.aIndex >= 0:
            self._filesModel.setA(offset + playlist.aIndex)
        for b in playlist.bIndexes:
            self._filesModel.setB(offset + b, True)
        self._filesModel.compareOptions = playlist.compareOptions
        self._filesModel.compareTime = playlist.compareTime
        if report:
            self.context.getSystemByName("ftk::LogSystem").print(
                "djv.App",
                "{0}: {1}".format(path.getFileName(), ", ".join(report)),
                ftk.LogType.Warning)
        self._recentPlaylistsModel.addRecent(path)

    def openPlaylistDialog(self):
        """
        Open the dialog for choosing a playlist to open.
        """
        selfWeak = weakref.ref(self)
        options = ftk.FileBrowserOpenOptions()
        options.title = "Open Playlist"
        options.extensions = [".otio"]
        options.extensionsLabel = "Playlists"
        self.context.getSystemByName("ftk::FileBrowserSystem").open(
            self._window,
            lambda path: selfWeak() and selfWeak().openPlaylist(path),
            options)

    def savePlaylist(self, path):
        """
        Save the file list as a ".otio" playlist.
        """
        path = path if isinstance(path, ftk.Path) else ftk.Path(str(path))
        playlist = djv.models.Playlist()
        playlist.items = list(self._filesModel.files)
        # The active file's position and in/out points live in the player
        # until the file loses focus, so bring its item up to date before
        # it is written.
        if self._activeFiles:
            player = self._player.get()
            if player:
                self._activeFiles[0].speed = player.speed
                self._activeFiles[0].currentTime = player.currentTime
                self._activeFiles[0].inOutRange = player.inOutRange
        playlist.aIndex = self._filesModel.aIndex
        playlist.bIndexes = list(self._filesModel.bIndexes)
        playlist.compareOptions = self._filesModel.compareOptions
        playlist.compareTime = self._filesModel.compareTime
        fileName = path.getFileName(True)
        if path.ext.lower() != ".otio":
            fileName += ".otio"
        djv.models.playlistSave(
            fileName,
            playlist,
            self._settingsModel.imageSeq.io.defaultSpeed)
        self._recentPlaylistsModel.addRecent(ftk.Path(fileName))

    def savePlaylistDialog(self):
        """
        Open the dialog for choosing where to save a playlist.
        """
        selfWeak = weakref.ref(self)
        options = ftk.FileBrowserOpenOptions()
        options.title = "Save Playlist"
        options.mode = ftk.FileBrowserMode.Save
        options.fileName = "playlist.otio"
        options.extensions = [".otio"]
        options.extensionsLabel = "Playlists"
        self.context.getSystemByName("ftk::FileBrowserSystem").open(
            self._window,
            lambda path: selfWeak() and selfWeak().savePlaylist(path),
            options)

    def openReview(self, path):
        """
        Open a review, replacing the current session.
        """
        path = str(path)
        try:
            review = djv.models.reviewOpen(path)
        except RuntimeError as e:
            self._log(str(e), ftk.LogType.Error)
            return
        for section in review.unreadSections:
            self._log(
                "Review \"{}\": the \"{}\" section could not be read "
                "and was left as it was.".format(path, section),
                ftk.LogType.Warning)
        self._applyReview(review, os.path.dirname(path), path)

    def _applyReview(self, review, base, reviewPath):
        pathOptions = ftk.PathOptions()
        pathOptions.seqMaxDigits = self._settingsModel.imageSeq.maxDigits

        # Replace the current session.
        self._filesModel.closeAll()

        missing = []
        for rf in review.files:
            resolved, exists = djv.models.resolveReviewPath(
                rf.path, rf.pathAbsolute, base, "", pathOptions)
            if not exists:
                missing.append(str(resolved))
            item = djv.models.FilesModelItem()
            item.id = rf.id if rf.id else djv.models.generateId()
            item.path = ftk.Path(str(resolved), pathOptions)
            if rf.audioPath or rf.audioPathAbsolute:
                audio, audioExists = djv.models.resolveReviewPath(
                    rf.audioPath, rf.audioPathAbsolute, base, "", pathOptions)
                if not audioExists:
                    missing.append(str(audio))
                item.audioPath = ftk.Path(str(audio), pathOptions)
            item.videoLayer = max(0, rf.videoLayer)
            item.speed = rf.speed
            item.currentTime = rf.currentTime
            item.inOutRange = rf.inOutRange
            # Add directly rather than through open(), which would
            # re-expand a directory entry into multiple files.
            self._filesModel.add(item)

        # Rebuild the comparison. Order matters: setting the compare
        # options may pick a "B" of its own when none is set, so clear
        # and rebuild "B" after it, then set "A".
        files = self._filesModel.files
        def indexOfId(id):
            for i, item in enumerate(files):
                if item.id == id:
                    return i
            return -1
        self._filesModel.compareOptions = review.compare.options
        self._filesModel.clearB()
        for bId in review.compare.bIds:
            index = indexOfId(bId)
            if index >= 0:
                self._filesModel.setB(index, True)
        aIndex = indexOfId(review.compare.aId)
        if aIndex < 0 and files:
            aIndex = 0
        if aIndex >= 0:
            self._filesModel.setA(aIndex)
        self._filesModel.compareTime = review.compare.time

        # Color and image display.
        self._colorModel.ocioOptions = review.color.ocio
        self._colorModel.lutOptions = review.color.lut
        self._viewportModel.displayOptions = review.color.display
        self._viewportModel.backgroundOptions = review.color.background
        self._viewportModel.foregroundOptions = review.color.foreground
        self._viewportModel.aspectRatioOptions = review.color.aspectRatio
        self._viewportModel.hudOptions = review.color.hud

        # Interface.
        self._toolsModel.closeTools()
        for tool in review.ui.openTools:
            self._toolsModel.setToolOpen(tool, True)

        self._notesModel.setNotes(review.notes)
        self._rangesModel.setRanges(review.ranges)
        self._annotationsModel.setAnnotations(review.annotations)

        # View state is applied once the new player's initial auto-frame
        # has settled.
        self._pendingReviewView = review.view
        self._applyReviewView()

        self._reviewPath = reviewPath
        self._reviewCarry = review
        self._recentReviewsModel.addRecent(ftk.Path(reviewPath))
        self._updateWindowTitle()

        if missing:
            self._log(
                "Review \"{}\": {} file(s) not found: {}".format(
                    reviewPath, len(missing), ", ".join(missing)),
                ftk.LogType.Warning)

    def _applyReviewView(self):
        view = self._pendingReviewView
        if view is None:
            return
        viewport = self._window.getViewport()
        if view.frameView:
            viewport.frameView = True
            self._pendingReviewView = None
        else:
            # Defer past the initial auto-frame that the new player
            # triggers on the next layout pass. Setting the position and
            # zoom disables frame view, so no later re-frame overrides it.
            selfWeak = weakref.ref(self)
            def apply():
                self_ = selfWeak()
                if self_ is None or self_._pendingReviewView is None:
                    return
                view = self_._pendingReviewView
                self_._window.getViewport().setViewPosAndZoom(
                    view.pos, view.zoom)
                self_._pendingReviewView = None
            self._reviewViewTimer = ftk.Timer(self.context)
            self._reviewViewTimer.start(0.2, apply)

    def _buildReview(self, base):
        review = djv.models.Review()
        review.app = "{} {}".format(
            self._appInfoModel.fullName, self._appInfoModel.version)
        review.created = djv.models.timestamp()
        # Carry what the review we last loaded held but we could not
        # use, so the save does not replace it with what we fell back
        # to.
        if self._reviewCarry is not None:
            review.carryUnread(self._reviewCarry)

        files = self._filesModel.files
        reviewFiles = []
        for item in files:
            rf = djv.models.ReviewFile()
            rf.id = item.id
            rf.pathAbsolute = djv.models.reviewGenericPath(
                item.path.getFileName(True))
            rf.path = djv.models.reviewRelativePath(
                item.path.getFileName(True), base)
            # The separate audio travels with the review like the file
            # does: stored absolute only, it would not survive the move.
            audio = item.audioPath.getFileName(True)
            rf.audioPath = djv.models.reviewRelativePath(audio, base)
            rf.audioPathAbsolute = djv.models.reviewGenericPath(audio)
            rf.videoLayer = item.videoLayer
            rf.speed = item.speed
            rf.currentTime = item.currentTime
            rf.inOutRange = item.inOutRange
            reviewFiles.append(rf)

        # Persist the live playback state of the active file, which the
        # model item only receives when the file is switched away from.
        player = self._player.get()
        aIndex = self._filesModel.aIndex
        if player and 0 <= aIndex < len(reviewFiles):
            reviewFiles[aIndex].speed = player.speed
            reviewFiles[aIndex].currentTime = player.currentTime
            reviewFiles[aIndex].inOutRange = player.inOutRange
        review.files = reviewFiles

        compare = djv.models.ReviewCompare()
        if 0 <= aIndex < len(files):
            compare.aId = files[aIndex].id
        bIds = []
        for bIndex in self._filesModel.bIndexes:
            if 0 <= bIndex < len(files):
                bIds.append(files[bIndex].id)
        compare.bIds = bIds
        compare.options = self._filesModel.compareOptions
        compare.time = self._filesModel.compareTime
        review.compare = compare

        viewport = self._window.getViewport()
        view = djv.models.ReviewView()
        view.frameView = viewport.frameView
        view.pos = viewport.viewPos
        view.zoom = viewport.zoom
        review.view = view

        color = djv.models.ReviewColor()
        color.ocio = self._colorModel.ocioOptions
        color.lut = self._colorModel.lutOptions
        color.display = self._viewportModel.displayOptions
        color.background = self._viewportModel.backgroundOptions
        color.foreground = self._viewportModel.foregroundOptions
        color.aspectRatio = self._viewportModel.aspectRatioOptions
        color.hud = self._viewportModel.hudOptions
        review.color = color

        ui = djv.models.ReviewUI()
        ui.openTools = self._toolsModel.openTools
        review.ui = ui

        review.notes = self._notesModel.notes
        review.ranges = self._rangesModel.ranges
        review.annotations = self._annotationsModel.annotations

        return review

    def saveReview(self, path = None):
        """
        Save the current session as a review. Without a path, the
        review's own path is used, and without one of those the save
        dialog is shown.
        """
        if path is None:
            if self._reviewPath is None:
                self.saveReviewAs()
                return
            path = self._reviewPath
        path = str(path)
        review = self._buildReview(os.path.dirname(path))
        try:
            djv.models.reviewSave(path, review)
        except RuntimeError as e:
            self._log(str(e), ftk.LogType.Error)
            return
        self._reviewPath = path
        self._reviewCarry = review
        self._recentReviewsModel.addRecent(ftk.Path(path))
        self._updateWindowTitle()

    def saveReviewAs(self):
        """
        Open the dialog for choosing where to save a review.
        """
        selfWeak = weakref.ref(self)
        def callback(path):
            self_ = selfWeak()
            if self_ is None:
                return
            fileName = path.getFileName(True)
            ext = djv.models.reviewExtension()
            if path.ext.lower() != ext:
                # Auto-complete the extension when the user types a
                # bare name.
                fileName = os.path.splitext(fileName)[0] + ext
            self_.saveReview(fileName)
        self._reviewFileDialog(ftk.FileBrowserMode.Save, "Save Review", callback)

    def openReviewDialog(self):
        """
        Open the dialog for choosing a review to open.
        """
        selfWeak = weakref.ref(self)
        self._reviewFileDialog(
            ftk.FileBrowserMode.Open,
            "Open Review",
            lambda path: selfWeak() and selfWeak().openReview(
                path.getFileName(True)))

    def _reviewFileDialog(self, mode, title, callback):
        options = ftk.FileBrowserOpenOptions()
        options.title = title
        if self._reviewPath is not None:
            options.path = os.path.dirname(self._reviewPath)
        options.mode = mode
        if ftk.FileBrowserMode.Save == mode:
            # The review's own name where there is one, the way the
            # playlists suggest "playlist.otio".
            options.fileName = (
                os.path.basename(self._reviewPath)
                if self._reviewPath is not None
                else "review" + djv.models.reviewExtension())
        options.extensions = [djv.models.reviewExtension()]
        options.extensionsLabel = "Review Session"
        self.context.getSystemByName("ftk::FileBrowserSystem").open(
            self._window, callback, options)

    def closeReview(self):
        """
        Close the review and reset to the startup state.
        """
        self._filesModel.closeAll()
        compareOptions = tl.CompareOptions()
        compareOptions.compare = tl.Compare._None
        self._filesModel.compareOptions = compareOptions
        self._notesModel.clear()
        self._rangesModel.clear()
        self._annotationsModel.clear()
        self._reviewPath = None
        self._reviewCarry = None
        self._updateWindowTitle()

    def getReviewPath(self):
        return self._reviewPath

    def getReviewMarkers(self):
        return list(self._reviewMarkers.get())

    def observeReviewMarkers(self):
        """
        Observe the frames that carry a note or a drawing, sorted.
        """
        return self._reviewMarkers

    def seekReviewMarker(self, next):
        """
        Go to the next or previous frame with a note or a drawing.
        """
        markers = self._reviewMarkers.get()
        player = self._player.get()
        if not markers or player is None:
            return
        currentTime = player.currentTime
        current = int(currentTime.value)
        if next:
            # The first marker strictly after the playhead, or wrap
            # around to the first one so the button never becomes a
            # dead end.
            following = [m for m in markers if m > current]
            target = following[0] if following else markers[0]
        else:
            preceding = [m for m in markers if m < current]
            target = preceding[-1] if preceding else markers[-1]
        player.stop()
        targetTime = otio.opentime.RationalTime(target, currentTime.rate)
        # Going to feedback wins over a narrower in/out range: with the
        # target outside it, the seek would move the clock into a span
        # the player cannot show.
        if not player.inOutRange.contains(targetTime):
            player.resetInPoint()
            player.resetOutPoint()
        player.currentTime = targetTime

    def _reviewMarkersUpdate(self):
        # Mark the frames that carry a note or a drawing in the
        # timeline. The markers are deliberately undifferentiated --
        # they say "there is something here" -- and follow the
        # timeline, which shows "A".
        markers = set()
        for note in self._notesModel.notes:
            if note.time is not None:
                markers.add(int(note.time.value))
        # Every annotation is stamped with the player's time, which is
        # the timeline's own clock, so a drawing made on a "B" source
        # still marks the right place.
        for annotation in self._annotationsModel.annotations:
            if annotation.time is not None:
                markers.add(int(annotation.time.value))
        markers = sorted(markers)
        self._reviewMarkers.setIfChanged(markers)
        self._window.getTimelineWidget().frameMarkers = markers

    def _updateWindowTitle(self):
        title = self._appInfoModel.title
        if self._reviewPath is not None:
            # Show the active review so the user can tell which one is
            # open. The name rather than the path, the way document
            # titles usually read; the Recent Reviews menu is where the
            # whole paths are.
            title += " - " + os.path.basename(self._reviewPath)
        self._window.title = title

    def _log(self, message, logType):
        self.context.getSystemByName("ftk::LogSystem").print(
            "djv.App", message, logType)

    def _parseEnum(self, name, value, enums):
        for e in enums:
            if tl.getLabel(e).lower() == value.lower():
                return e
        raise RuntimeError(
            "Cannot parse the {}: \"{}\", expected one of: {}".format(
                name, value, ", ".join(tl.getLabel(e) for e in enums)))

    def _inputFilesInit(self):
        inputs = list(self._cmdLineInputs.list)
        if not inputs:
            return

        # A review (".djvr") describes an entire session; open it and
        # ignore any other inputs.
        if os.path.splitext(inputs[0])[1] == djv.models.reviewExtension():
            self.openReview(inputs[0])
            return

        pathOptions = ftk.PathOptions()
        pathOptions.seqMaxDigits = self._settingsModel.imageSeq.maxDigits

        if self._cmdLineCompare.hasValue:
            path = ftk.Path(self._cmdLineCompare.value, pathOptions)
            if path.hasSeqWildcard:
                path = ftk.expandSeq(path, pathOptions)
            self.open(path)
            options = self._filesModel.compareOptions
            if self._cmdLineCompareMode.hasValue:
                options.compare = self._parseEnum(
                    "compare mode",
                    self._cmdLineCompareMode.value,
                    tl.getCompareEnums())
            if self._cmdLineWipeCenter.hasValue:
                parts = self._cmdLineWipeCenter.value.replace(",", " ").split()
                options.wipeCenter = ftk.V2F(float(parts[0]), float(parts[1]))
            if self._cmdLineWipeRotation.hasValue:
                options.wipeRotation = self._cmdLineWipeRotation.value
            self._filesModel.compareOptions = options
            self._filesModel.setB(0, True)

        audioPath = None
        if self._cmdLineAudio.hasValue:
            audioPath = ftk.Path(self._cmdLineAudio.value)
        frameRange = None
        if self._cmdLineFrameRange.hasValue:
            frameRange = djv.models.parseFrameRange(
                self._cmdLineFrameRange.value)

        for value in inputs:
            path = ftk.Path(value, pathOptions)
            if path.hasSeqWildcard:
                path = ftk.expandSeq(path, pathOptions)
            self.open(path, audioPath, frameRange)
            # Only the first file opened takes the range.
            frameRange = None

            player = self._player.get()
            if player is None:
                continue
            if self._cmdLineSpeed.hasValue:
                player.speed = self._cmdLineSpeed.value
            if self._cmdLineTimeUnits.hasValue:
                self._timeUnitsModel.timeUnits = self._parseEnum(
                    "time units",
                    self._cmdLineTimeUnits.value,
                    tl.getTimeUnitsEnums())
            speed = player.speed
            timeUnits = self._timeUnitsModel.timeUnits
            if self._cmdLineInPoint.hasValue:
                inOutRange = \
                    otio.opentime.TimeRange.range_from_start_end_time_inclusive(
                        djv.models.parseTime(
                            "in point",
                            self._cmdLineInPoint.value,
                            speed,
                            timeUnits),
                        player.inOutRange.end_time_inclusive())
                player.inOutRange = inOutRange
                player.currentTime = inOutRange.start_time
            if self._cmdLineOutPoint.hasValue:
                inOutRange = \
                    otio.opentime.TimeRange.range_from_start_end_time_inclusive(
                        player.inOutRange.start_time,
                        djv.models.parseTime(
                            "out point",
                            self._cmdLineOutPoint.value,
                            speed,
                            timeUnits))
                player.inOutRange = inOutRange
                player.currentTime = inOutRange.start_time
            if self._cmdLineSeek.hasValue:
                player.currentTime = djv.models.parseTime(
                    "seek time",
                    self._cmdLineSeek.value,
                    speed,
                    timeUnits)
            if self._cmdLineLoop.hasValue:
                player.loop = self._parseEnum(
                    "loop mode",
                    self._cmdLineLoop.value,
                    tl.getLoopEnums())
            if self._cmdLinePlayback.hasValue:
                player.playback = self._parseEnum(
                    "playback mode",
                    self._cmdLinePlayback.value,
                    tl.getPlaybackEnums())

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
        self._annotationsModel = djv.models.AnnotationsModel()
        self._drawModel = djv.models.DrawModel(self._settings)
        self._notesModel = djv.models.NotesModel()
        self._rangesModel = djv.models.RangesModel()
        self._recentReviewsModel = djv.models.RecentFilesModel(
            self.context, self._settings, "Review")
        self._recentPlaylistsModel = djv.models.RecentFilesModel(
            self.context, self._settings, "Playlist")
        self._sysLogModel = ftk.SysLogModel(self.context)
        self._commandsModel = djv.models.CommandsModel(self.context)

        self._player = tl.ObservablePlayer(None)
        self._files = []
        self._timelines = []
        self._activeFiles = []
        self._reviewPath = None
        self._reviewCarry = None
        self._pendingReviewView = None
        self._reviewViewTimer = None
        self._reviewMarkers = ftk.ObservableIntList([])

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
        self._notesMarkersObserver = djv.models.ReviewNoteListObserver(
            self._notesModel.observeNotes,
            lambda value: selfWeak()._reviewMarkersUpdate())
        self._annotationsMarkersObserver = \
            djv.models.ReviewAnnotationListObserver(
                self._annotationsModel.observeAnnotations,
                lambda value: selfWeak()._reviewMarkersUpdate())

        if self._cmdLineSysInfo.found:
            print("\n".join(djv.ui.getSysInfo(
                self.context, self._appInfoModel, self._settingsModel)))
            return

        # Color options apply to the session whether or not a file was
        # named.
        if (self._cmdLineOCIO.hasValue or
                self._cmdLineOCIOInput.hasValue or
                self._cmdLineOCIODisplay.hasValue or
                self._cmdLineOCIOView.hasValue or
                self._cmdLineOCIOLook.hasValue):
            options = self._colorModel.ocioOptions
            options.enabled = True
            if self._cmdLineOCIO.hasValue:
                options.fileName = self._cmdLineOCIO.value
            if self._cmdLineOCIOInput.hasValue:
                options.input = self._cmdLineOCIOInput.value
            if self._cmdLineOCIODisplay.hasValue:
                options.display = self._cmdLineOCIODisplay.value
            if self._cmdLineOCIOView.hasValue:
                options.view = self._cmdLineOCIOView.value
            if self._cmdLineOCIOLook.hasValue:
                options.look = self._cmdLineOCIOLook.value
            self._colorModel.ocioOptions = options
        if self._cmdLineLUT.hasValue or self._cmdLineLUTOrder.hasValue:
            options = self._colorModel.lutOptions
            options.enabled = True
            if self._cmdLineLUT.hasValue:
                options.fileName = self._cmdLineLUT.value
            if self._cmdLineLUTOrder.hasValue:
                options.order = self._parseEnum(
                    "LUT operation order",
                    self._cmdLineLUTOrder.value,
                    tl.getLUTOrderEnums())
            self._colorModel.lutOptions = options

        self._inputFilesInit()

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
        self._recentReviewsModel.save()
        self._recentPlaylistsModel.save()
        self._viewportModel.save()
        self._colorModel.save()
        self._audioModel.save()
        self._toolsModel.save()
        self._settingsModel.save()
        self.settings.save()

    def _commandTimeout(self):
        self._commandTicks += 1
        if not self._cmdLineInputs.list or \
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
