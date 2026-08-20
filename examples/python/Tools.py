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
    def __init__(self, context, app, name, objectName, parent = None):
        ftk.IContainer.__init__(self, context, objectName, parent)

        self._app = weakref.ref(app)
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
    def __init__(self, context, app, parent = None):
        IToolWidget.__init__(
            self, context, app, "Information", "InfoTool", parent)

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
    def __init__(self, context, app, parent = None):
        IToolWidget.__init__(self, context, app, "Files", "FilesTool", parent)

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
        for row, item in enumerate(files):
            nameButton = ftk.ToolButton(
                self.context, item.path.fileName, self._grid)
            nameButton.checked = item is a
            nameButton.hStretch = ftk.Stretch.Expanding
            nameButton.tooltip = item.path.get() + "\n\nSet the A file."
            self._aButtonGroup.addButton(nameButton)
            self._grid.setGridPos(nameButton, row, 0)

            bButton = ftk.ToolButton(self.context, "B", self._grid)
            bButton.checked = any(item is i for i in b)
            bButton.tooltip = "Set the B file(s)."
            self._bButtonGroup.addButton(bButton)
            self._grid.setGridPos(bButton, row, 1)

            layerComboBox = ftk.ComboBox(self.context, self._grid)
            layerComboBox.setItems(item.videoLayers)
            layerComboBox.currentIndex = item.videoLayer
            layerComboBox.tooltip = "Set the current layer."
            layerComboBox.setVisible(len(item.videoLayers) > 1)
            layerComboBox.setIndexCallback(
                lambda value, captured = item, appWeak = self._app:
                    appWeak().getFilesModel().setLayer(captured, value))
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
            lambda value, f = Util.weak(self._setBackground):
                f(appWeak(), "type", tl.getBackgroundEnums()[value]))
        self._solidSwatch.setCallback(
            lambda value, f = Util.weak(self._setBackground):
                f(appWeak(), "solidColor", value))
        self._checkers0Swatch.setCallback(
            lambda value, f = Util.weak(self._setCheckersColor):
                f(appWeak(), 0, value))
        self._checkers1Swatch.setCallback(
            lambda value, f = Util.weak(self._setCheckersColor):
                f(appWeak(), 1, value))
        self._gridCheckBox.setCheckedCallback(
            lambda value, f = Util.weak(self._setGrid):
                f(appWeak(), "enabled", value))
        self._gridSizeSlider.setCallback(
            lambda value, f = Util.weak(self._setGrid):
                f(appWeak(), "cellSize", value))
        self._gridColorSwatch.setCallback(
            lambda value, f = Util.weak(self._setGrid):
                f(appWeak(), "color", value))
        self._hudCheckBox.setCheckedCallback(
            lambda value, f = Util.weak(self._setHUD):
                f(appWeak(), value))

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

class ColorTool(IToolWidget):
    """
    This tool provides the color management and adjustments.
    """
    def __init__(self, context, app, parent = None):
        IToolWidget.__init__(self, context, app, "Color", "ColorTool", parent)

        colorModel = app.getColorModel()
        # The tool owns the OCIO model and syncs it both ways with the
        # color model, the way the C++ color widgets do it.
        self._ocioModel = djv.models.OCIOModel(context)
        ocioModel = self._ocioModel
        layout = ftk.VerticalLayout(context)
        layout.spacingRole = ftk.SizeRole._None
        appWeak = weakref.ref(app)
        selfWeak = weakref.ref(self)

        # OCIO.
        self._ocioEnabledCheckBox = ftk.CheckBox(context)
        self._ocioConfigComboBox = ftk.ComboBox(
            context, [tl.getLabel(c) for c in tl.getOCIOConfigEnums()])
        self._ocioConfigComboBox.hStretch = ftk.Stretch.Expanding
        self._ocioFileEdit = ftk.FileEdit(context)
        self._ocioInputComboBox = ftk.ComboBox(context)
        self._ocioInputComboBox.hStretch = ftk.Stretch.Expanding
        self._ocioDisplayComboBox = ftk.ComboBox(context)
        self._ocioDisplayComboBox.hStretch = ftk.Stretch.Expanding
        self._ocioViewComboBox = ftk.ComboBox(context)
        self._ocioViewComboBox.hStretch = ftk.Stretch.Expanding
        self._ocioLookComboBox = ftk.ComboBox(context)
        self._ocioLookComboBox.hStretch = ftk.Stretch.Expanding
        vLayout = ftk.VerticalLayout(context)
        vLayout.marginRole = ftk.SizeRole.Margin
        form = ftk.FormLayout(context, vLayout)
        form.spacingRole = ftk.SizeRole.SpacingSmall
        form.addRow("Config:", self._ocioConfigComboBox)
        form.addRow("File:", self._ocioFileEdit)
        form.addRow("Input:", self._ocioInputComboBox)
        form.addRow("Display:", self._ocioDisplayComboBox)
        form.addRow("View:", self._ocioViewComboBox)
        form.addRow("Look:", self._ocioLookComboBox)
        bellows = ftk.Bellows(context, "OCIO", layout)
        bellows.widget = vLayout
        bellows.toolWidget = self._ocioEnabledCheckBox

        self._ocioEnabledCheckBox.setCheckedCallback(
            lambda value: selfWeak()._ocioModel.setEnabled(value))
        self._ocioConfigComboBox.setIndexCallback(
            lambda value: selfWeak()._ocioModel.setConfig(
                tl.getOCIOConfigEnums()[value]))
        self._ocioFileEdit.setCallback(
            lambda value: selfWeak()._ocioModel.setFileName(str(value)))
        self._ocioInputComboBox.setIndexCallback(
            lambda value: selfWeak()._ocioModel.setInputIndex(value))
        self._ocioDisplayComboBox.setIndexCallback(
            lambda value: selfWeak()._ocioModel.setDisplayIndex(value))
        self._ocioViewComboBox.setIndexCallback(
            lambda value: selfWeak()._ocioModel.setViewIndex(value))
        self._ocioLookComboBox.setIndexCallback(
            lambda value: selfWeak()._ocioModel.setLookIndex(value))
        self._ocioDataObserver = djv.models.OCIOModelDataObserver(
            ocioModel.observeData,
            lambda value: selfWeak()._ocioDataUpdate(value))
        self._ocioOptionsObserver = djv.models.OCIOOptionsObserver(
            colorModel.observeOCIOOptions,
            lambda value: selfWeak()._ocioModel.setOptions(value))
        self._ocioOptionsObserver2 = djv.models.OCIOOptionsObserver(
            ocioModel.observeOptions,
            lambda value: setattr(
                appWeak().getColorModel(), "ocioOptions", value))

        # LUT.
        self._lutEnabledCheckBox = ftk.CheckBox(context)
        self._lutFileEdit = ftk.FileEdit(context)
        self._lutOrderComboBox = ftk.ComboBox(
            context, [tl.getLabel(o) for o in tl.getLUTOrderEnums()])
        self._lutOrderComboBox.hStretch = ftk.Stretch.Expanding
        vLayout = ftk.VerticalLayout(context)
        vLayout.marginRole = ftk.SizeRole.Margin
        form = ftk.FormLayout(context, vLayout)
        form.spacingRole = ftk.SizeRole.SpacingSmall
        form.addRow("File:", self._lutFileEdit)
        form.addRow("Order:", self._lutOrderComboBox)
        bellows = ftk.Bellows(context, "LUT", layout)
        bellows.widget = vLayout
        bellows.toolWidget = self._lutEnabledCheckBox

        self._lutEnabledCheckBox.setCheckedCallback(
            lambda value, f = Util.weak(self._setLUT):
                f(appWeak(), "enabled", value))
        self._lutFileEdit.setCallback(
            lambda value, f = Util.weak(self._setLUT):
                f(appWeak(), "fileName", str(value)))
        self._lutOrderComboBox.setIndexCallback(
            lambda value, f = Util.weak(self._setLUT):
                f(appWeak(), "order", tl.getLUTOrderEnums()[value]))
        self._lutObserver = djv.models.LUTOptionsObserver(
            colorModel.observeLUTOptions,
            lambda value: selfWeak()._lutUpdate(value))

        # The color adjustments drive the viewport model's display
        # options. The color values are per channel; the sliders set the
        # channels together.
        self._displaySliders = {}
        self._displayChecks = {}
        for section, fields in [
            ("Color", [
                ("brightness", 0.0, 4.0, True),
                ("contrast", 0.0, 4.0, True),
                ("saturation", 0.0, 4.0, True),
                ("hue", -180.0, 180.0, False)]),
            ("Levels", [
                ("inLow", 0.0, 1.0, False),
                ("inHigh", 0.0, 1.0, False),
                ("gamma", 0.1, 4.0, False),
                ("outLow", 0.0, 1.0, False),
                ("outHigh", 0.0, 1.0, False)]),
            ("Exposure", [
                ("exposure", -10.0, 10.0, False),
                ("defog", 0.0, 0.1, False),
                ("kneeLow", -3.0, 3.0, False),
                ("kneeHigh", 3.5, 7.5, False),
                ("gamma", 0.1, 4.0, False)]),
            ("SoftClip", [
                ("value", 0.0, 1.0, False)]),
        ]:
            check = ftk.CheckBox(context)
            self._displayChecks[section] = check
            check.setCheckedCallback(
                lambda value, captured = section, \
                    f = Util.weak(self._setDisplay):
                    f(appWeak(), captured, "enabled", value))
            vLayout = ftk.VerticalLayout(context)
            vLayout.marginRole = ftk.SizeRole.Margin
            form = ftk.FormLayout(context, vLayout)
            form.spacingRole = ftk.SizeRole.SpacingSmall
            for field, lo, hi, vec in fields:
                slider = ftk.FloatEditSlider(context)
                slider.range = ftk.RangeF(lo, hi)
                slider.setCallback(
                    lambda value, s = section, f = field, v = vec, \
                        fn = Util.weak(self._setDisplay):
                        fn(appWeak(), s, f,
                           ftk.V3F(value, value, value) if v else value))
                form.addRow(field + ":", slider)
                self._displaySliders[(section, field)] = (slider, vec)
            title = "Soft Clip" if section == "SoftClip" else section
            bellows = ftk.Bellows(context, title, layout)
            bellows.widget = vLayout
            bellows.toolWidget = check

        self._setContent(layout)

        self._displayObserver = djv.models.DisplayOptionsObserver(
            app.getViewportModel().observeDisplayOptions,
            lambda value: selfWeak()._displayUpdate(value))

    def _ocioDataUpdate(self, data):
        self._ocioEnabledCheckBox.checked = data.enabled
        self._ocioConfigComboBox.currentIndex = \
            tl.getOCIOConfigEnums().index(data.config)
        self._ocioFileEdit.path = data.fileName
        for comboBox, items, index in [
            (self._ocioInputComboBox, data.inputs, data.inputIndex),
            (self._ocioDisplayComboBox, data.displays, data.displayIndex),
            (self._ocioViewComboBox, data.views, data.viewIndex),
            (self._ocioLookComboBox, data.looks, data.lookIndex)]:
            comboBox.setItems(items)
            comboBox.currentIndex = index

    def _setLUT(self, app, name, value):
        options = app.getColorModel().lutOptions
        setattr(options, name, value)
        app.getColorModel().lutOptions = options

    def _lutUpdate(self, options):
        self._lutEnabledCheckBox.checked = options.enabled
        self._lutFileEdit.path = options.fileName
        self._lutOrderComboBox.currentIndex = \
            tl.getLUTOrderEnums().index(options.order)

    def _setDisplay(self, app, section, name, value):
        options = app.getViewportModel().displayOptions
        part = getattr(options, section[0].lower() + section[1:])
        setattr(part, name, value)
        setattr(options, section[0].lower() + section[1:], part)
        app.getViewportModel().displayOptions = options

    def _displayUpdate(self, options):
        for section, part in [
            ("Color", options.color),
            ("Levels", options.levels),
            ("Exposure", options.exposure),
            ("SoftClip", options.softClip)]:
            self._displayChecks[section].checked = part.enabled
            for (s, field), (slider, vec) in self._displaySliders.items():
                if s == section:
                    value = getattr(part, field)
                    slider.value = value.x if vec else value

class MessagesTool(IToolWidget):
    """
    This tool displays the warning and error messages.
    """
    def __init__(self, context, app, parent = None):
        IToolWidget.__init__(
            self, context, app, "Messages", "MessagesTool", parent)

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
    def __init__(self, context, app, parent = None):
        IToolWidget.__init__(
            self, context, app, "System Log", "SysLogTool", parent)

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
    def __init__(self, context, app, parent = None):
        IToolWidget.__init__(
            self, context, app, "Settings", "SettingsTool", parent)

        settingsModel = app.getSettingsModel()
        layout = ftk.VerticalLayout(context)
        layout.spacingRole = ftk.SizeRole._None
        appWeak = weakref.ref(app)
        selfWeak = weakref.ref(self)

        def bellowsForm(title):
            vLayout = ftk.VerticalLayout(context)
            vLayout.marginRole = ftk.SizeRole.Margin
            form = ftk.FormLayout(context, vLayout)
            form.spacingRole = ftk.SizeRole.SpacingSmall
            bellows = ftk.Bellows(context, title, layout)
            bellows.widget = vLayout
            return form

        def setSetting(prop, field, value):
            model = appWeak().getSettingsModel()
            settings = getattr(model, prop)
            setattr(settings, field, value)
            setattr(model, prop, settings)

        # Cache.
        form = bellowsForm("Cache")
        self._cacheVideoSlider = ftk.FloatEditSlider(context)
        self._cacheVideoSlider.range = ftk.RangeF(0.0, 64.0)
        self._cacheVideoSlider.setCallback(
            lambda value: setSetting("cache", "videoGB", value))
        form.addRow("Video GB:", self._cacheVideoSlider)
        self._cacheAudioSlider = ftk.FloatEditSlider(context)
        self._cacheAudioSlider.range = ftk.RangeF(0.0, 64.0)
        self._cacheAudioSlider.setCallback(
            lambda value: setSetting("cache", "audioGB", value))
        form.addRow("Audio GB:", self._cacheAudioSlider)
        self._cacheBehindSlider = ftk.FloatEditSlider(context)
        self._cacheBehindSlider.range = ftk.RangeF(0.0, 60.0)
        self._cacheBehindSlider.setCallback(
            lambda value: setSetting("cache", "readBehind", value))
        form.addRow("Read behind:", self._cacheBehindSlider)

        # File browser.
        form = bellowsForm("File Browser")
        self._nativeFileDialogCheckBox = ftk.CheckBox(context)
        self._nativeFileDialogCheckBox.setCheckedCallback(
            lambda value: setSetting("fileBrowser", "nativeFileDialog", value))
        form.addRow("Native file dialog:", self._nativeFileDialogCheckBox)

        # Image sequences.
        form = bellowsForm("Image Sequences")
        self._seqAudioComboBox = ftk.ComboBox(
            context, [tl.getLabel(a) for a in tl.getImageSeqAudioEnums()])
        self._seqAudioComboBox.hStretch = ftk.Stretch.Expanding
        self._seqAudioComboBox.setIndexCallback(
            lambda value: setSetting(
                "imageSeq", "audio", tl.getImageSeqAudioEnums()[value]))
        form.addRow("Audio:", self._seqAudioComboBox)
        self._seqAudioFileNameEdit = ftk.LineEdit(context)
        self._seqAudioFileNameEdit.setCallback(
            lambda value: setSetting("imageSeq", "audioFileName", value))
        form.addRow("Audio file name:", self._seqAudioFileNameEdit)
        self._seqMaxDigitsSlider = ftk.IntEditSlider(context)
        self._seqMaxDigitsSlider.range = ftk.RangeI(1, 16)
        self._seqMaxDigitsSlider.setCallback(
            lambda value: setSetting("imageSeq", "maxDigits", value))
        form.addRow("Maximum digits:", self._seqMaxDigitsSlider)
        self._seqThreadsSlider = ftk.IntEditSlider(context)
        self._seqThreadsSlider.range = ftk.RangeI(1, 64)
        self._seqThreadsSlider.setCallback(
            lambda value: setSetting("imageSeq", "readThreadCount", value))
        form.addRow("Read threads:", self._seqThreadsSlider)

        # Mouse.
        form = bellowsForm("Mouse")
        self._wheelScaleSlider = ftk.FloatEditSlider(context)
        self._wheelScaleSlider.range = ftk.RangeF(1.0, 2.0)
        self._wheelScaleSlider.setCallback(
            lambda value: setSetting("mouse", "wheelScale", value))
        form.addRow("Wheel scale:", self._wheelScaleSlider)

        # Playback.
        form = bellowsForm("Playback")
        self._startPlaybackCheckBox = ftk.CheckBox(context)
        self._startPlaybackCheckBox.setCheckedCallback(
            lambda value: setSetting("playback", "startPlayback", value))
        form.addRow("Start on open:", self._startPlaybackCheckBox)

        # Style.
        form = bellowsForm("Style")
        self._displayScaleSlider = ftk.FloatEditSlider(context)
        self._displayScaleSlider.range = ftk.RangeF(0.0, 4.0)
        self._displayScaleSlider.step = 0.25
        self._displayScaleSlider.tooltip = "The display scale; zero is automatic."
        self._displayScaleSlider.setCallback(
            lambda value: setSetting("style", "displayScale", value))
        form.addRow("Display scale:", self._displayScaleSlider)

        # Time.
        form = bellowsForm("Time")
        self._timeUnitsComboBox = ftk.ComboBox(
            context, [tl.getLabel(u) for u in tl.getTimeUnitsEnums()])
        self._timeUnitsComboBox.hStretch = ftk.Stretch.Expanding
        self._timeUnitsComboBox.setIndexCallback(
            lambda value: setattr(
                appWeak().getTimeUnitsModel(),
                "timeUnits",
                tl.getTimeUnitsEnums()[value]))
        form.addRow("Units:", self._timeUnitsComboBox)

        # Miscellaneous.
        form = bellowsForm("Miscellaneous")
        self._tooltipsCheckBox = ftk.CheckBox(context)
        self._tooltipsCheckBox.setCheckedCallback(
            lambda value: setSetting("misc", "tooltipsEnabled", value))
        form.addRow("Tooltips:", self._tooltipsCheckBox)

        resetButton = ftk.PushButton(context, "Reset")
        resetButton.tooltip = "Reset the settings to their defaults."
        resetButton.setClickedCallback(
            lambda: appWeak().getSettingsModel().reset())
        vLayout = ftk.VerticalLayout(context, layout)
        vLayout.marginRole = ftk.SizeRole.Margin
        resetButton.parent = vLayout

        self._setContent(layout)

        self._cacheObserver = tl.PlayerCacheOptionsObserver(
            settingsModel.observeCache,
            lambda value: selfWeak()._cacheUpdate(value))
        self._fileBrowserObserver = djv.models.FileBrowserSettingsObserver(
            settingsModel.observeFileBrowser,
            lambda value: selfWeak()._fileBrowserUpdate(value))
        self._imageSeqObserver = djv.models.ImageSeqSettingsObserver(
            settingsModel.observeImageSeq,
            lambda value: selfWeak()._imageSeqUpdate(value))
        self._mouseObserver = djv.models.MouseSettingsObserver(
            settingsModel.observeMouse,
            lambda value: selfWeak()._mouseUpdate(value))
        self._playbackObserver = djv.models.PlaybackSettingsObserver(
            settingsModel.observePlayback,
            lambda value: selfWeak()._playbackUpdate(value))
        self._styleObserver = djv.models.StyleSettingsObserver(
            settingsModel.observeStyle,
            lambda value: selfWeak()._styleUpdate(value))
        self._miscObserver = djv.models.MiscSettingsObserver(
            settingsModel.observeMisc,
            lambda value: selfWeak()._miscUpdate(value))

    def _cacheUpdate(self, value):
        self._cacheVideoSlider.value = value.videoGB
        self._cacheAudioSlider.value = value.audioGB
        self._cacheBehindSlider.value = value.readBehind

    def _fileBrowserUpdate(self, value):
        self._nativeFileDialogCheckBox.checked = value.nativeFileDialog

    def _imageSeqUpdate(self, value):
        self._seqAudioComboBox.currentIndex = \
            tl.getImageSeqAudioEnums().index(value.audio)
        self._seqAudioFileNameEdit.text = value.audioFileName
        self._seqMaxDigitsSlider.value = value.maxDigits
        self._seqThreadsSlider.value = value.readThreadCount

    def _mouseUpdate(self, value):
        self._wheelScaleSlider.value = value.wheelScale

    def _playbackUpdate(self, value):
        self._startPlaybackCheckBox.checked = value.startPlayback

    def _styleUpdate(self, value):
        self._displayScaleSlider.value = value.displayScale

    def _miscUpdate(self, value):
        self._tooltipsCheckBox.checked = value.tooltipsEnabled

# The tools this application implements so far; the tools model lists
# more, and the actions only offer what can actually open.
FACTORY = {
    "Files": FilesTool,
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
    def __init__(self, context, app, parent = None):
        ftk.IContainer.__init__(self, context, "ToolsWidget", parent)

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
                widget = FACTORY[name](self.context, self._app())
                widget.parent = self._layout
                self._widgets[name] = widget
        self.setVisible(len(self._widgets) > 0)
