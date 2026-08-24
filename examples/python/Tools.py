# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the DJV project.

import opentimelineio as otio
import ftkPy as ftk
import tlRenderPy as tl
import djvPy as djv

import Util

import weakref

class IToolWidget(ftk.IContainer):
    """
    Base class for tool widgets: a title row with a close button, and
    the tool's content below it.
    """
    def __init__(self, context, app, mainWindow, name, objectName, parent = None):
        ftk.IContainer.__init__(self, context, objectName, parent)

        self._app = weakref.ref(app)
        self._mainWindow = weakref.ref(mainWindow)
        self.name = name

        self._label = ftk.Label(context, name)
        self._label.marginRole = ftk.SizeRole.MarginSmall
        self._label.hStretch = ftk.Stretch.Expanding

        self._closeButton = ftk.ToolButton(context)
        self._closeButton.icon = "Close"
        self._closeButton.setClickedCallback(Util.weak(self._close))

        self._layout = ftk.VerticalLayout(context)
        self._layout.spacingRole = ftk.SizeRole._None
        hLayout = ftk.HorizontalLayout(context, self._layout)
        hLayout.spacingRole = ftk.SizeRole._None
        self._label.parent = hLayout
        self._closeButton.parent = hLayout
        ftk.Divider(context, ftk.Orientation.Vertical, self._layout)
        self._setWidget(self._layout)

    def _setContent(self, widget):
        widget.parent = self._layout

    def _close(self):
        if self._app():
            self._app().getToolsModel().setToolOpen(self.name, False)

class InfoTool(IToolWidget):
    """
    This tool displays information about the current file.
    """
    def __init__(self, context, app, mainWindow, parent = None):
        IToolWidget.__init__(
            self, context, app, mainWindow, "Information", "InfoTool", parent)

        self._formLayout = ftk.FormLayout(context)
        self._formLayout.marginRole = ftk.SizeRole.MarginSmall
        self._formLayout.spacingRole = ftk.SizeRole.SpacingSmall
        self._setContent(self._formLayout)

        selfWeak = weakref.ref(self)
        self._playerObserver = tl.PlayerObserver(
            app.observePlayer(),
            lambda player: selfWeak()._playerUpdate(player))

    def _playerUpdate(self, player):
        self._formLayout.clear()
        if player:
            rows = [("Name", player.path.get())]
            ioInfo = player.ioInfo
            if ioInfo.video:
                video = ioInfo.video[0]
                rows.append(("Video", "{}x{}:{:.2f} {}".format(
                    video.size.w,
                    video.size.h,
                    video.aspect,
                    ftk.to_string(video.type))))
            if ioInfo.audio.isValid:
                rows.append(("Audio", "{} {} {}".format(
                    ioInfo.audio.channelCount,
                    tl.to_string(ioInfo.audio.type),
                    ioInfo.audio.sampleRate)))
            for tag in sorted(ioInfo.tags):
                rows.append((tag, ioInfo.tags[tag]))
            for name, text in rows:
                label = ftk.Label(self.context, text)
                self._formLayout.addRow(name + ":", label)

class AudioTool(IToolWidget):
    """
    This tool provides the audio controls.
    """
    def __init__(self, context, app, mainWindow, parent = None):
        IToolWidget.__init__(self, context, app, mainWindow, "Audio", "AudioTool", parent)

        audioModel = app.getAudioModel()

        self._deviceComboBox = ftk.ComboBox(context)
        self._deviceComboBox.tooltip = "The audio device."

        self._volumeSlider = ftk.FloatEditSlider(context)
        self._volumeSlider.range = ftk.RangeF(0.0, 1.0)
        self._volumeSlider.step = 0.1
        self._volumeSlider.largeStep = 0.25
        self._volumeSlider.tooltip = "The audio volume."

        self._muteCheckBox = ftk.CheckBox(context)
        self._muteCheckBox.tooltip = "Mute the audio."

        self._syncOffsetSlider = ftk.DoubleEditSlider(context)
        self._syncOffsetSlider.range = ftk.RangeD(-1.0, 1.0)
        self._syncOffsetSlider.step = 0.01
        self._syncOffsetSlider.largeStep = 0.1
        self._syncOffsetSlider.tooltip = "The audio sync offset in seconds."

        self._formLayout = ftk.FormLayout(context)
        self._formLayout.marginRole = ftk.SizeRole.MarginSmall
        self._formLayout.spacingRole = ftk.SizeRole.SpacingSmall
        self._formLayout.addRow("Device:", self._deviceComboBox)
        self._formLayout.addRow("Volume:", self._volumeSlider)
        self._formLayout.addRow("Mute:", self._muteCheckBox)
        self._formLayout.addRow("Sync offset:", self._syncOffsetSlider)
        self._setContent(self._formLayout)

        appWeak = weakref.ref(app)
        self._deviceComboBox.setIndexCallback(
            lambda index, f = Util.weak(self._deviceCallback):
                f(appWeak(), index))
        self._volumeSlider.setCallback(
            lambda value: setattr(appWeak().getAudioModel(), "volume", value))
        self._muteCheckBox.setCheckedCallback(
            lambda value: setattr(appWeak().getAudioModel(), "mute", value))
        self._syncOffsetSlider.setCallback(
            lambda value: setattr(appWeak().getAudioModel(), "syncOffset", value))

        selfWeak = weakref.ref(self)
        self._devicesObserver = djv.models.AudioDeviceIDListObserver(
            audioModel.observeDevices,
            lambda devices: selfWeak()._devicesUpdate(devices))
        self._deviceObserver = djv.models.AudioDeviceIDObserver(
            audioModel.observeDevice,
            lambda device: selfWeak()._deviceUpdate(device))
        self._volumeObserver = ftk.FloatObserver(
            audioModel.observeVolume,
            lambda value: selfWeak()._volumeUpdate(value))
        self._muteObserver = ftk.BoolObserver(
            audioModel.observeMute,
            lambda value: selfWeak()._muteUpdate(value))
        self._syncOffsetObserver = ftk.DoubleObserver(
            audioModel.observeSyncOffset,
            lambda value: selfWeak()._syncOffsetUpdate(value))

    def _deviceCallback(self, app, index):
        devices = app.getAudioModel().devices
        if 0 <= index and index < len(devices):
            app.getAudioModel().device = devices[index]

    def _devicesUpdate(self, devices):
        self._devices = list(devices)
        self._deviceComboBox.setItems([device.name for device in devices])

    def _deviceUpdate(self, device):
        for i, existing in enumerate(getattr(self, "_devices", [])):
            if existing == device:
                self._deviceComboBox.currentIndex = i

    def _volumeUpdate(self, value):
        self._volumeSlider.value = value

    def _muteUpdate(self, value):
        self._muteCheckBox.checked = value

    def _syncOffsetUpdate(self, value):
        self._syncOffsetSlider.value = value

class FilesTool(IToolWidget):
    """
    This tool manages the open files and the comparison options.
    """
    def __init__(self, context, app, mainWindow, parent = None):
        IToolWidget.__init__(self, context, app, mainWindow, "Files", "FilesTool", parent)

        self._rowWidgets = []
        self._rangePopup = None
        self._rangeItem = None
        self._rangeValue = None
        self._rangeTimer = ftk.Timer(context)

        self._aButtonGroup = ftk.ButtonGroup(context, ftk.ButtonGroupType.Radio)
        self._bButtonGroup = ftk.ButtonGroup(context, ftk.ButtonGroupType.Check)

        layout = ftk.VerticalLayout(context)
        layout.spacingRole = ftk.SizeRole._None

        self._grid = ftk.GridLayout(context, layout)
        self._grid.setSpacingRole(ftk.SizeRole.SpacingSmall, ftk.SizeRole._None)
        self._grid.rowBackgroundRole = ftk.ColorRole.Header

        ftk.Divider(context, ftk.Orientation.Vertical, layout)

        # The comparison options.
        self._compareComboBox = ftk.ComboBox(
            context, [tl.getLabel(mode) for mode in tl.getCompareEnums()])
        self._compareComboBox.hStretch = ftk.Stretch.Expanding

        self._wipeXSlider = ftk.FloatEditSlider(context)
        self._wipeYSlider = ftk.FloatEditSlider(context)
        self._wipeRotationSlider = ftk.FloatEditSlider(context)
        self._wipeRotationSlider.range = ftk.RangeF(0.0, 360.0)
        self._wipeRotationSlider.step = 1.0
        self._wipeRotationSlider.largeStep = 10.0

        self._overlaySlider = ftk.FloatEditSlider(context)
        self._overlaySlider.defaultValue = 0.5

        self._gainSlider = ftk.FloatEditSlider(context)
        self._gainSlider.range = ftk.RangeF(1.0, 32.0)
        self._gainSlider.step = 1.0
        self._gainSlider.largeStep = 4.0
        self._gainSlider.defaultValue = 1.0
        self._gainSlider.tooltip = (
            "Multiply the difference, so that a small one can be seen.\n"
            "A compressed file differs from its source by a code value\n"
            "or two, which is not distinguishable from black on its own.")

        self._compareTimeComboBox = ftk.ComboBox(
            context, djv.models.getCompareTimeLabels())
        self._compareTimeComboBox.hStretch = ftk.Stretch.Expanding
        self._compareTimeComboBox.tooltip = (
            "Which frame of each file is shown together: the same frame\n"
            "counted from the start of each, or the same timecode.")

        self._sameSizeCheckBox = ftk.CheckBox(context)
        self._sameSizeCheckBox.hStretch = ftk.Stretch.Expanding
        self._sameSizeCheckBox.tooltip = (
            "Draw the compared files at the size of the current file,\n"
            "so a smaller one is not shown tiny beside it.")

        vLayout = ftk.VerticalLayout(context)
        vLayout.marginRole = ftk.SizeRole.Margin
        form = ftk.FormLayout(context, vLayout)
        form.spacingRole = ftk.SizeRole.SpacingSmall
        form.addRow("Mode:", self._compareComboBox)
        form.addRow("X:", self._wipeXSlider)
        form.addRow("Y:", self._wipeYSlider)
        form.addRow("Rotation:", self._wipeRotationSlider)
        form.addRow("Amount:", self._overlaySlider)
        form.addRow("Gain:", self._gainSlider)
        form.addRow("Sync by:", self._compareTimeComboBox)
        form.addRow("Same size:", self._sameSizeCheckBox)
        self._compareBellows = ftk.Bellows(context, "Compare", layout)
        self._compareBellows.widget = vLayout

        self._setContent(layout)

        appWeak = weakref.ref(app)
        self._aButtonGroup.setCheckedCallback(
            lambda index, value: appWeak().getFilesModel().setA(index))
        self._bButtonGroup.setCheckedCallback(
            lambda index, value: appWeak().getFilesModel().setB(index, value))
        self._compareComboBox.setIndexCallback(
            lambda value, f = Util.weak(self._setCompareOption):
                f(appWeak(), "compare", tl.getCompareEnums()[value]))
        self._wipeXSlider.setCallback(
            lambda value, f = Util.weak(self._setWipeCenter):
                f(appWeak(), x = value))
        self._wipeYSlider.setCallback(
            lambda value, f = Util.weak(self._setWipeCenter):
                f(appWeak(), y = value))
        self._wipeRotationSlider.setCallback(
            lambda value, f = Util.weak(self._setCompareOption):
                f(appWeak(), "wipeRotation", value))
        self._overlaySlider.setCallback(
            lambda value, f = Util.weak(self._setCompareOption):
                f(appWeak(), "overlay", value))
        self._gainSlider.setCallback(
            lambda value, f = Util.weak(self._setCompareOption):
                f(appWeak(), "differenceGain", value))
        self._compareTimeComboBox.setIndexCallback(
            lambda value: setattr(
                appWeak().getFilesModel(),
                "compareTime",
                tl.getCompareTimeEnums()[value]))
        self._sameSizeCheckBox.setCheckedCallback(
            lambda value, f = Util.weak(self._setCompareOption):
                f(appWeak(), "sameSize", value))

        selfWeak = weakref.ref(self)
        self._filesObserver = djv.models.FilesModelItemListObserver(
            app.getFilesModel().observeFiles,
            lambda files: selfWeak()._filesUpdate(files))
        self._aIndexObserver = ftk.IntObserver(
            app.getFilesModel().observeAIndex,
            lambda value: selfWeak()._aIndexUpdate(value))
        self._bObserver = ftk.IntListObserver(
            app.getFilesModel().observeBIndexes,
            lambda indexes: selfWeak()._bUpdate(indexes))
        self._layersObserver = ftk.IntListObserver(
            app.getFilesModel().observeLayers,
            lambda layers: selfWeak()._layersUpdate(layers))
        self._compareObserver = djv.models.CompareOptionsObserver(
            app.getFilesModel().observeCompareOptions,
            lambda value: selfWeak()._compareUpdate(value))
        self._compareTimeObserver = djv.models.CompareTimeObserver(
            app.getFilesModel().observeCompareTime,
            lambda value: selfWeak()._compareTimeUpdate(value))

    def _setCompareOption(self, app, name, value):
        options = app.getFilesModel().compareOptions
        setattr(options, name, value)
        app.getFilesModel().compareOptions = options

    def _setWipeCenter(self, app, x = None, y = None):
        options = app.getFilesModel().compareOptions
        center = options.wipeCenter
        if x is not None:
            center.x = x
        if y is not None:
            center.y = y
        options.wipeCenter = center
        app.getFilesModel().compareOptions = options

    def _filesUpdate(self, files):
        self._rowWidgets = []
        self._aButtonGroup.clearButtons()
        self._bButtonGroup.clearButtons()
        self._grid.clear()
        app = self._app()
        a = app.getFilesModel().a
        b = app.getFilesModel().b
        seqExts = tl.getExts(app.context, int(tl.FileType.Seq))
        for row, item in enumerate(files):
            thumbnail = djv.ui.FileThumbnail(
                self.context, item,
                app.getSettingsModel().ioOptions, self._grid)
            self._grid.setGridPos(thumbnail, row, 0)

            nameButton = ftk.ToolButton(
                self.context, item.path.fileName, self._grid)
            nameButton.checked = item is a
            nameButton.hStretch = ftk.Stretch.Expanding
            nameButton.tooltip = item.path.get() + "\n\nSet the A file."
            self._aButtonGroup.addButton(nameButton)
            self._grid.setGridPos(nameButton, row, 1)

            bButton = ftk.ToolButton(self.context, "B", self._grid)
            bButton.checked = any(item is i for i in b)
            bButton.tooltip = "Set the B file(s)."
            self._bButtonGroup.addButton(bButton)
            self._grid.setGridPos(bButton, row, 2)

            layerComboBox = ftk.ComboBox(self.context, self._grid)
            layerComboBox.setItems(item.videoLayers)
            layerComboBox.currentIndex = item.videoLayer
            layerComboBox.tooltip = "Set the current layer."
            layerComboBox.setVisible(len(item.videoLayers) > 1)
            layerComboBox.setIndexCallback(
                lambda value, captured = item, appWeak = self._app:
                    appWeak().getFilesModel().setLayer(captured, value))
            self._grid.setGridPos(layerComboBox, row, 3)

            # Only an image sequence has a frame range to state. The range
            # is what the sequence is meant to cover, which need not be
            # what is on disk yet.
            if item.path.hasNum and item.path.testExt(seqExts):
                if item.timeRange is not None:
                    start = int(item.timeRange.start_time().value())
                    duration = int(item.timeRange.duration().value())
                    frames = ftk.RangeI64(start, start + duration - 1)
                elif item.path.frames is not None:
                    frames = item.path.frames
                else:
                    frames = ftk.RangeI64(0, 0)
                rangeButton = ftk.ToolButton(
                    self.context,
                    "{}-{}".format(frames.min, frames.max),
                    self._grid)
                rangeButton.tooltip = "The frame range of the sequence."
                rangeButton.setClickedCallback(
                    lambda captured = item, r = frames, index = row, \
                        f = Util.weak(self._showRangePopup):
                        f(captured, r, index))
                self._grid.setGridPos(rangeButton, row, 4)
            else:
                rangeButton = None

            self._rowWidgets.append(
                (nameButton, bButton, layerComboBox, rangeButton))

    def _showRangePopup(self, item, frames, row):
        if self._rangePopup or row >= len(self._rowWidgets):
            return
        button = self._rowWidgets[row][3]
        if button is None:
            return
        self._rangePopup = djv.ui.FrameRangePopup(self.context, frames)
        selfWeak = weakref.ref(self)
        self._rangePopup.setCallback(
            lambda value, captured = item:
                selfWeak()._rangeUpdate(captured, value))
        self._rangePopup.open(self.window, button.geometry, None)
        self._rangePopup.setCloseCallback(
            lambda: setattr(selfWeak(), "_rangePopup", None))

    def _rangeUpdate(self, item, value):
        self._rangeItem = item
        self._rangeValue = value
        # Restarted on each change, so holding a spin box down reopens the
        # file once, at the range it is left on, rather than at every value
        # passed through on the way there.
        self._rangeTimer.start(0.5, Util.weak(self._rangeTimeout))

    def _rangeTimeout(self):
        if self._rangeItem is not None:
            self._app().getFilesModel().setFrames(
                self._rangeItem, self._rangeValue)
            self._rangeItem = None

    def _aIndexUpdate(self, value):
        self._aButtonGroup.setChecked(value, True)

    def _bUpdate(self, indexes):
        for i, widgets in enumerate(self._rowWidgets):
            widgets[1].checked = i in indexes

    def _layersUpdate(self, layers):
        for i, widgets in enumerate(self._rowWidgets):
            if i < len(layers):
                widgets[2].currentIndex = layers[i]

    def _compareUpdate(self, options):
        self._compareComboBox.currentIndex = \
            tl.getCompareEnums().index(options.compare)
        self._wipeXSlider.value = options.wipeCenter.x
        self._wipeYSlider.value = options.wipeCenter.y
        self._wipeRotationSlider.value = options.wipeRotation
        self._overlaySlider.value = options.overlay
        self._gainSlider.value = options.differenceGain
        self._sameSizeCheckBox.checked = options.sameSize

    def _compareTimeUpdate(self, value):
        self._compareTimeComboBox.currentIndex = \
            tl.getCompareTimeEnums().index(value)

class ViewTool(IToolWidget):
    """
    This tool provides the view options.
    """
    def __init__(self, context, app, mainWindow, parent = None):
        IToolWidget.__init__(self, context, app, mainWindow, "View", "ViewTool", parent)

        viewportModel = app.getViewportModel()
        layout = ftk.VerticalLayout(context)
        layout.spacingRole = ftk.SizeRole._None
        for title, widget in [
            ("Options", djv.ui.ViewOptionsWidget(context, viewportModel)),
            ("Aspect Ratio", djv.ui.ViewAspectRatioWidget(context, viewportModel)),
            ("Background", djv.ui.ViewBackgroundWidget(context, viewportModel)),
            ("Outline", djv.ui.ViewOutlineWidget(context, viewportModel)),
            ("Grid", djv.ui.ViewGridWidget(context, viewportModel)),
            ("Center Marker", djv.ui.ViewCenterMarkerWidget(context, viewportModel)),
            ("HUD", djv.ui.ViewHUDWidget(context, viewportModel)),
        ]:
            bellows = ftk.Bellows(context, title, layout)
            bellows.widget = widget
        self._setContent(layout)

class ColorTool(IToolWidget):
    """
    This tool provides the color management and adjustments.
    """
    def __init__(self, context, app, mainWindow, parent = None):
        IToolWidget.__init__(self, context, app, mainWindow, "Color", "ColorTool", parent)

        colorModel = app.getColorModel()
        viewportModel = app.getViewportModel()
        layout = ftk.VerticalLayout(context)
        layout.spacingRole = ftk.SizeRole._None
        for title, widget in [
            ("OCIO", djv.ui.OCIOWidget(context, colorModel)),
            ("LUT", djv.ui.LUTWidget(context, colorModel)),
            ("Color", djv.ui.ColorWidget(context, viewportModel)),
            ("Levels", djv.ui.LevelsWidget(context, app.settings, viewportModel)),
            ("Exposure", djv.ui.ExposureWidget(context, viewportModel)),
            ("Soft Clip", djv.ui.SoftClipWidget(context, viewportModel)),
        ]:
            bellows = ftk.Bellows(context, title, layout)
            bellows.widget = widget
            bellows.toolWidget = widget.enabledCheckBox
        self._setContent(layout)

class MessagesTool(IToolWidget):
    """
    This tool displays the warning and error messages.
    """
    def __init__(self, context, app, mainWindow, parent = None):
        IToolWidget.__init__(
            self, context, app, mainWindow, "Messages", "MessagesTool", parent)

        sysLogModel = app.getSysLogModel()

        self._label = ftk.Label(context)
        self._label.marginRole = ftk.SizeRole.MarginSmall
        self._label.vStretch = ftk.Stretch.Expanding

        clearButton = ftk.ToolButton(context, "Clear")
        clearButton.tooltip = "Clear the messages."
        appWeak = weakref.ref(app)
        clearButton.setClickedCallback(
            lambda: appWeak().getSysLogModel().clearMessages())

        layout = ftk.VerticalLayout(context)
        layout.spacingRole = ftk.SizeRole.SpacingSmall
        self._label.parent = layout
        clearButton.parent = layout
        self._setContent(layout)

        selfWeak = weakref.ref(self)
        self._messagesObserver = ftk.LogItemListObserver(
            sysLogModel.observeMessages(),
            lambda items: selfWeak()._messagesUpdate(items))

    def _messagesUpdate(self, items):
        self._label.text = "\n".join(
            "{}: {}".format(ftk.getLabel(item.type), item.message)
            for item in items)

class SysLogTool(IToolWidget):
    """
    This tool displays the system log.
    """
    def __init__(self, context, app, mainWindow, parent = None):
        IToolWidget.__init__(
            self, context, app, mainWindow, "System Log", "SysLogTool", parent)

        sysLogModel = app.getSysLogModel()

        self._label = ftk.Label(context)
        self._label.marginRole = ftk.SizeRole.MarginSmall
        self._label.font = ftk.FontType.Mono
        self._label.vStretch = ftk.Stretch.Expanding

        clearButton = ftk.ToolButton(context, "Clear")
        clearButton.tooltip = "Clear the log."
        appWeak = weakref.ref(app)
        clearButton.setClickedCallback(
            lambda: appWeak().getSysLogModel().clearLog())

        layout = ftk.VerticalLayout(context)
        layout.spacingRole = ftk.SizeRole.SpacingSmall
        self._label.parent = layout
        clearButton.parent = layout
        self._setContent(layout)

        selfWeak = weakref.ref(self)
        self._logObserver = ftk.StringListObserver(
            sysLogModel.observeLog(),
            lambda lines: selfWeak()._logUpdate(lines))

    def _logUpdate(self, lines):
        self._label.text = "\n".join(lines)

class SettingsTool(IToolWidget):
    """
    This tool provides the settings.
    """
    def __init__(self, context, app, mainWindow, parent = None):
        IToolWidget.__init__(
            self, context, app, mainWindow, "Settings", "SettingsTool", parent)

        settingsModel = app.getSettingsModel()
        layout = ftk.VerticalLayout(context)
        layout.spacingRole = ftk.SizeRole.Border
        newFileNote = "Changes are applied to new files."
        sections = [
            ("Cache", [djv.ui.CacheSettingsWidget(context, settingsModel)]),
            ("File Browser",
             [djv.ui.FileBrowserSettingsWidget(context, settingsModel)]),
            ("Image Sequences",
             [djv.ui.ImageSeqSettingsWidget(
                 context, settingsModel, app.getViewportModel())]),
            ("OTIO", [newFileNote,
                      djv.ui.OTIOSettingsWidget(context, settingsModel)]),
            ("Mouse", [djv.ui.MouseSettingsWidget(context, settingsModel)]),
            ("Playback",
             [djv.ui.PlaybackSettingsWidget(context, settingsModel)]),
            ("Audio", [newFileNote,
                       djv.ui.AudioSettingsWidget(context, settingsModel)]),
            ("Keyboard Shortcuts",
             [djv.ui.ShortcutsSettingsWidget(context, settingsModel)]),
            ("Style", [djv.ui.StyleSettingsWidget(context, settingsModel)]),
            ("Time",
             [djv.ui.TimeSettingsWidget(context, app.getTimeUnitsModel())]),
        ]
        # The FFmpeg widgets follow the build, like the C++ application.
        if hasattr(djv.ui, "FFmpegSettingsWidget"):
            sections.append(("FFmpeg", [
                newFileNote,
                djv.ui.FFmpegSettingsWidget(context, settingsModel)]))
        if hasattr(djv.ui, "FFmpegCmdSettingsWidget"):
            sections.append(("FFmpeg Command", [
                newFileNote,
                djv.ui.FFmpegCmdSettingsWidget(context, settingsModel)]))
        sections.append(
            ("Miscellaneous",
             [djv.ui.MiscSettingsWidget(context, settingsModel)]))
        self._bellows = {}
        for title, widgets in sections:
            vLayout = ftk.VerticalLayout(context)
            vLayout.marginRole = ftk.SizeRole.Margin
            for widget in widgets:
                if isinstance(widget, str):
                    ftk.Label(context, widget, vLayout)
                else:
                    widget.parent = vLayout
            bellows = ftk.Bellows(context, title, layout)
            bellows.widget = vLayout
            self._bellows[title] = bellows

        ftk.Divider(context, ftk.Orientation.Vertical, layout)
        hLayout = ftk.HorizontalLayout(context, layout)
        hLayout.marginRole = ftk.SizeRole.MarginSmall
        hLayout.spacingRole = ftk.SizeRole.SpacingSmall
        saveButton = ftk.PushButton(context, "Save", hLayout)
        saveButton.tooltip = \
            "Save the settings. Settings are also saved on exit."
        spacer = ftk.Spacer(context, ftk.Orientation.Horizontal, hLayout)
        spacer.hStretch = ftk.Stretch.Expanding
        resetButton = ftk.PushButton(context, "Reset", hLayout)
        resetButton.tooltip = "Restore settings to default values."
        self._setContent(layout)

        appWeak = weakref.ref(app)
        saveButton.setClickedCallback(
            lambda: appWeak().getSettingsModel().save())
        resetButton.setClickedCallback(Util.weak(self._reset))

    def _reset(self):
        appWeak = self._app
        dialogSystem = \
            appWeak().context.getSystemByName("ftk::DialogSystem")
        dialogSystem.confirm(
            "Reset Settings",
            "Reset settings to default values?",
            self.window,
            lambda value:
                appWeak().getSettingsModel().reset() if value else None)

class ColorPickerTool(IToolWidget):
    """
    This tool displays the picked color. The pick mouse action samples
    the viewport.
    """
    def __init__(self, context, app, mainWindow, parent = None):
        IToolWidget.__init__(
            self, context, app, mainWindow,
            "Color Picker", "ColorPickerTool", parent)

        self._colorSwatch = ftk.ColorSwatch(context)
        self._colorSwatch.color = ftk.Color4F(0.0, 0.0, 0.0)
        self._colorSwatch.border = False
        self._colorSwatch.sizeRole = ftk.SizeRole.SwatchLarge

        self._colorLabel = ftk.Label(context)
        self._colorLabel.font = ftk.FontType.Mono
        self._pixelLabel = ftk.Label(context)
        self._pixelLabel.font = ftk.FontType.Mono
        self._mouseLabel = ftk.Label(context)

        layout = ftk.VerticalLayout(context)
        layout.spacingRole = ftk.SizeRole._None
        self._colorSwatch.parent = layout
        form = ftk.FormLayout(context, layout)
        form.marginRole = ftk.SizeRole.Margin
        form.spacingRole = ftk.SizeRole.SpacingSmall
        form.addRow("Color:", self._colorLabel)
        form.addRow("Pixel:", self._pixelLabel)
        form.addRow("Mouse:", self._mouseLabel)
        self._setContent(layout)

        viewport = mainWindow.getViewport()
        selfWeak = weakref.ref(self)
        self._pickObserver = tl.ui.OptionalV2IObserver(
            viewport.observePick,
            lambda value: selfWeak()._pickUpdate(value))
        self._colorSampleObserver = tl.ui.OptionalColor4FObserver(
            viewport.observeColorSample,
            lambda value: selfWeak()._colorSampleUpdate(value))
        self._mouseSettingsObserver = djv.models.MouseSettingsObserver(
            app.getSettingsModel().observeMouse,
            lambda value: selfWeak()._mouseSettingsUpdate(value))

    def _pickUpdate(self, value):
        self._pixelLabel.text = \
            "{} {}".format(value.x, value.y) if value is not None else "-"

    def _colorSampleUpdate(self, value):
        self._colorSwatch.color = value if value is not None else ftk.Color4F()
        self._colorLabel.text = \
            "{:.2f} {:.2f} {:.2f} {:.2f}".format(
                value.r, value.g, value.b, value.a) \
            if value is not None else "-"

    def _mouseSettingsUpdate(self, settings):
        s = []
        binding = settings.bindings.get(djv.models.MouseAction.Pick)
        if binding is not None and binding.button != ftk.MouseButton._None:
            if binding.modifier != ftk.KeyModifier._None:
                s.append(ftk.to_string(binding.modifier))
            s.append(ftk.getLabel(binding.button))
        self._mouseLabel.text = "{} Click".format(" + ".join(s))

# The tools this application implements so far; the tools model lists
# more, and the actions only offer what can actually open.
FACTORY = {
    "Files": FilesTool,
    "Color Picker": ColorPickerTool,
    "View": ViewTool,
    "Color": ColorTool,
    "Information": InfoTool,
    "Audio": AudioTool,
    "Settings": SettingsTool,
    "Messages": MessagesTool,
    "System Log": SysLogTool,
}

class ToolsWidget(ftk.IContainer):
    """
    This widget holds the open tools.
    """
    def __init__(self, context, app, mainWindow, parent = None):
        ftk.IContainer.__init__(self, context, "ToolsWidget", parent)

        self._app = weakref.ref(app)
        self._mainWindow = weakref.ref(mainWindow)
        self._widgets = {}

        self._layout = ftk.VerticalLayout(context)
        self._layout.spacingRole = ftk.SizeRole.SpacingSmall

        # One scroll area for the whole stack rather than one inside
        # each tool, so a tool takes the height its contents need.
        self._scrollWidget = ftk.ScrollWidget(context, ftk.ScrollType.Both)
        self._scrollWidget.border = False
        self._scrollWidget.widget = self._layout
        self._setWidget(self._scrollWidget)

        selfWeak = weakref.ref(self)
        self._openObserver = ftk.StringListObserver(
            app.getToolsModel().observeOpenTools,
            lambda names: selfWeak()._openUpdate(names))

    def _openUpdate(self, names):
        for name, widget in list(self._widgets.items()):
            if name not in names:
                widget.parent = None
                del self._widgets[name]
        for name in names:
            if name not in self._widgets and name in FACTORY:
                widget = FACTORY[name](
                    self.context, self._app(), self._mainWindow())
                widget.parent = self._layout
                self._widgets[name] = widget
        self.setVisible(len(self._widgets) > 0)
