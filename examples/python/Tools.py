# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the DJV project.

import opentimelineio as otio
import ftkPy as ftk
import tlRenderPy as tl
import djvPy as djv

import weakref

class IToolWidget(ftk.IContainer):
    """
    Base class for tool widgets: a title row with a close button, and
    the tool's content below it.
    """
    def __init__(self, context, app, name, objectName, parent = None):
        ftk.IContainer.__init__(self, context, objectName, parent)

        self._app = weakref.ref(app)
        self.name = name

        self._label = ftk.Label(context, name)
        self._label.marginRole = ftk.SizeRole.MarginSmall
        self._label.hStretch = ftk.Stretch.Expanding

        self._closeButton = ftk.ToolButton(context)
        self._closeButton.icon = "Close"
        self._closeButton.setClickedCallback(self._close)

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
    def __init__(self, context, app, parent = None):
        IToolWidget.__init__(
            self, context, app, "Information", "InfoTool", parent)

        self._formLayout = ftk.FormLayout(context)
        self._formLayout.marginRole = ftk.SizeRole.MarginSmall
        self._formLayout.spacingRole = ftk.SizeRole.SpacingSmall
        self._setContent(self._formLayout)
        self._context = context

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
                label = ftk.Label(self._context, text)
                self._formLayout.addRow(name + ":", label)

class AudioTool(IToolWidget):
    """
    This tool provides the audio controls.
    """
    def __init__(self, context, app, parent = None):
        IToolWidget.__init__(self, context, app, "Audio", "AudioTool", parent)

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
            lambda index: self._deviceCallback(appWeak(), index))
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
    def __init__(self, context, app, parent = None):
        IToolWidget.__init__(self, context, app, "Files", "FilesTool", parent)

        self._context = context
        self._rowWidgets = []

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
            lambda value: self._setCompareOption(
                appWeak(), "compare", tl.getCompareEnums()[value]))
        self._wipeXSlider.setCallback(
            lambda value: self._setWipeCenter(appWeak(), x = value))
        self._wipeYSlider.setCallback(
            lambda value: self._setWipeCenter(appWeak(), y = value))
        self._wipeRotationSlider.setCallback(
            lambda value: self._setCompareOption(appWeak(), "wipeRotation", value))
        self._overlaySlider.setCallback(
            lambda value: self._setCompareOption(appWeak(), "overlay", value))
        self._gainSlider.setCallback(
            lambda value: self._setCompareOption(appWeak(), "differenceGain", value))
        self._compareTimeComboBox.setIndexCallback(
            lambda value: setattr(
                appWeak().getFilesModel(),
                "compareTime",
                tl.getCompareTimeEnums()[value]))
        self._sameSizeCheckBox.setCheckedCallback(
            lambda value: self._setCompareOption(appWeak(), "sameSize", value))

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
        for row, item in enumerate(files):
            nameButton = ftk.ToolButton(
                self._context, item.path.fileName, self._grid)
            nameButton.checked = item is a
            nameButton.hStretch = ftk.Stretch.Expanding
            nameButton.tooltip = item.path.get() + "\n\nSet the A file."
            self._aButtonGroup.addButton(nameButton)
            self._grid.setGridPos(nameButton, row, 0)

            bButton = ftk.ToolButton(self._context, "B", self._grid)
            bButton.checked = any(item is i for i in b)
            bButton.tooltip = "Set the B file(s)."
            self._bButtonGroup.addButton(bButton)
            self._grid.setGridPos(bButton, row, 1)

            layerComboBox = ftk.ComboBox(self._context, self._grid)
            layerComboBox.setItems(item.videoLayers)
            layerComboBox.currentIndex = item.videoLayer
            layerComboBox.tooltip = "Set the current layer."
            layerComboBox.setVisible(len(item.videoLayers) > 1)
            layerComboBox.setIndexCallback(
                lambda value, captured = item:
                    self._app().getFilesModel().setLayer(captured, value))
            self._grid.setGridPos(layerComboBox, row, 2)

            self._rowWidgets.append((nameButton, bButton, layerComboBox))

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
    def __init__(self, context, app, parent = None):
        IToolWidget.__init__(self, context, app, "View", "ViewTool", parent)

        viewportModel = app.getViewportModel()
        layout = ftk.VerticalLayout(context)
        layout.spacingRole = ftk.SizeRole._None

        # Background.
        self._backgroundComboBox = ftk.ComboBox(
            context, [tl.getLabel(t) for t in tl.getBackgroundEnums()])
        self._backgroundComboBox.hStretch = ftk.Stretch.Expanding
        self._solidSwatch = ftk.ColorSwatch(context)
        self._solidSwatch.editable = True
        self._checkers0Swatch = ftk.ColorSwatch(context)
        self._checkers0Swatch.editable = True
        self._checkers1Swatch = ftk.ColorSwatch(context)
        self._checkers1Swatch.editable = True
        bgLayout = ftk.VerticalLayout(context)
        bgLayout.marginRole = ftk.SizeRole.Margin
        bgForm = ftk.FormLayout(context, bgLayout)
        bgForm.spacingRole = ftk.SizeRole.SpacingSmall
        bgForm.addRow("Type:", self._backgroundComboBox)
        bgForm.addRow("Solid color:", self._solidSwatch)
        bgForm.addRow("Checkers 0:", self._checkers0Swatch)
        bgForm.addRow("Checkers 1:", self._checkers1Swatch)
        bellows = ftk.Bellows(context, "Background", layout)
        bellows.widget = bgLayout

        # Grid.
        self._gridCheckBox = ftk.CheckBox(context)
        self._gridSizeSlider = ftk.IntEditSlider(context)
        self._gridSizeSlider.range = ftk.RangeI(1, 1000)
        self._gridColorSwatch = ftk.ColorSwatch(context)
        self._gridColorSwatch.editable = True
        gridLayout = ftk.VerticalLayout(context)
        gridLayout.marginRole = ftk.SizeRole.Margin
        gridForm = ftk.FormLayout(context, gridLayout)
        gridForm.spacingRole = ftk.SizeRole.SpacingSmall
        gridForm.addRow("Enabled:", self._gridCheckBox)
        gridForm.addRow("Size:", self._gridSizeSlider)
        gridForm.addRow("Color:", self._gridColorSwatch)
        bellows = ftk.Bellows(context, "Grid", layout)
        bellows.widget = gridLayout

        # HUD.
        self._hudCheckBox = ftk.CheckBox(context)
        hudLayout = ftk.VerticalLayout(context)
        hudLayout.marginRole = ftk.SizeRole.Margin
        hudForm = ftk.FormLayout(context, hudLayout)
        hudForm.spacingRole = ftk.SizeRole.SpacingSmall
        hudForm.addRow("Enabled:", self._hudCheckBox)
        bellows = ftk.Bellows(context, "HUD", layout)
        bellows.widget = hudLayout

        self._setContent(layout)

        appWeak = weakref.ref(app)
        self._backgroundComboBox.setIndexCallback(
            lambda value: self._setBackground(
                appWeak(), "type", tl.getBackgroundEnums()[value]))
        self._solidSwatch.setCallback(
            lambda value: self._setBackground(appWeak(), "solidColor", value))
        self._checkers0Swatch.setCallback(
            lambda value: self._setCheckersColor(appWeak(), 0, value))
        self._checkers1Swatch.setCallback(
            lambda value: self._setCheckersColor(appWeak(), 1, value))
        self._gridCheckBox.setCheckedCallback(
            lambda value: self._setGrid(appWeak(), "enabled", value))
        self._gridSizeSlider.setCallback(
            lambda value: self._setGrid(appWeak(), "cellSize", value))
        self._gridColorSwatch.setCallback(
            lambda value: self._setGrid(appWeak(), "color", value))
        self._hudCheckBox.setCheckedCallback(
            lambda value: self._setHUD(appWeak(), value))

        selfWeak = weakref.ref(self)
        self._bgObserver = djv.models.BackgroundOptionsObserver(
            viewportModel.observeBackgroundOptions,
            lambda value: selfWeak()._bgUpdate(value))
        self._fgObserver = djv.models.ForegroundOptionsObserver(
            viewportModel.observeForegroundOptions,
            lambda value: selfWeak()._fgUpdate(value))
        self._hudObserver = djv.models.HUDOptionsObserver(
            viewportModel.observeHUDOptions,
            lambda value: selfWeak()._hudUpdate(value))

    def _setBackground(self, app, name, value):
        options = app.getViewportModel().backgroundOptions
        setattr(options, name, value)
        app.getViewportModel().backgroundOptions = options

    def _setCheckersColor(self, app, index, value):
        options = app.getViewportModel().backgroundOptions
        colors = list(options.checkersColor)
        colors[index] = value
        options.checkersColor = tuple(colors)
        app.getViewportModel().backgroundOptions = options

    def _setGrid(self, app, name, value):
        options = app.getViewportModel().foregroundOptions
        grid = options.grid
        setattr(grid, name, value)
        options.grid = grid
        app.getViewportModel().foregroundOptions = options

    def _setHUD(self, app, value):
        options = app.getViewportModel().hudOptions
        options.enabled = value
        app.getViewportModel().hudOptions = options

    def _bgUpdate(self, options):
        self._backgroundComboBox.currentIndex = \
            tl.getBackgroundEnums().index(options.type)
        self._solidSwatch.color = options.solidColor
        self._checkers0Swatch.color = options.checkersColor[0]
        self._checkers1Swatch.color = options.checkersColor[1]

    def _fgUpdate(self, options):
        self._gridCheckBox.checked = options.grid.enabled
        self._gridSizeSlider.value = options.grid.cellSize
        self._gridColorSwatch.color = options.grid.color

    def _hudUpdate(self, options):
        self._hudCheckBox.checked = options.enabled

# The tools this application implements so far; the tools model lists
# more, and the actions only offer what can actually open.
FACTORY = {
    "Files": FilesTool,
    "View": ViewTool,
    "Information": InfoTool,
    "Audio": AudioTool,
}

class ToolsWidget(ftk.IContainer):
    """
    This widget holds the open tools.
    """
    def __init__(self, context, app, parent = None):
        ftk.IContainer.__init__(self, context, "ToolsWidget", parent)

        self._context = context
        self._app = weakref.ref(app)
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
                widget = FACTORY[name](self._context, self._app())
                widget.parent = self._layout
                self._widgets[name] = widget
        self.setVisible(len(self._widgets) > 0)
