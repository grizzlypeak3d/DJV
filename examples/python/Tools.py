# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the DJV project.

import opentimelineio as otio
import ftkPy as ftk
import tlRenderPy as tl
import djvPy as djv

import Util

import types
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
        # The settings are kept rather than reached through the
        # application, so the bellows can still be saved at exit when
        # the application is already gone.
        self._settings = app.settings
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

    def _loadBellows(self, bellows):
        for title, b in bellows.items():
            found, value = self._settings.getBool(
                "/{}/Bellows/{}".format(self.name, title))
            if found:
                b.open = value
        # The state is written when it changes; the write in __del__ is a
        # backstop, and __del__ is at the garbage collector's mercy when
        # something leaks.
        selfWeak = weakref.ref(self)
        for title, b in bellows.items():
            b.setOpenCallback(
                lambda value, captured = title:
                    selfWeak() and selfWeak()._settings.setBool(
                        "/{}/Bellows/{}".format(selfWeak().name, captured),
                        value))

    def _saveBellows(self, bellows):
        for title, b in bellows.items():
            self._settings.setBool(
                "/{}/Bellows/{}".format(self.name, title), b.open)

    def __del__(self):
        if hasattr(self, "_bellows"):
            self._saveBellows(self._bellows)

class InfoTool(IToolWidget):
    """
    This tool displays information about the current file.
    """
    def __init__(self, context, app, mainWindow, parent = None):
        IToolWidget.__init__(
            self, context, app, mainWindow, "Information", "InfoTool", parent)

        self._widget = djv.ui.InfoWidget(context, app.settings)
        self._setContent(self._widget)

        selfWeak = weakref.ref(self)
        self._playerObserver = tl.PlayerObserver(
            app.observePlayer(),
            lambda player: selfWeak()._widget.setPlayer(player))

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
        # An offset can legitimately exceed the starting range.
        self._syncOffsetSlider.getModel().rangeSoft = True
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
        # Where a dragged file would land: an index into the rows, counting
        # the gap after the last one, or -1 for nowhere.
        self._dropTarget = -1
        self._handle = 0
        self._rangePopup = None
        self._rangeItem = None
        self._rangeValue = None
        self._rangeTimer = ftk.Timer(context)

        self._bButtonGroup = ftk.ButtonGroup(context, ftk.ButtonGroupType.Check)

        layout = ftk.VerticalLayout(context)
        layout.spacingRole = ftk.SizeRole._None

        # The files are a list of items, like the markers and the file
        # browser: the list is the focus unit, the arrows browse it,
        # Return sets "A", Delete closes, and the whole row drags.
        self._fileList = ftk.ItemButtonList(context, layout)
        self._fileList.spacingRole = ftk.SizeRole._None

        ftk.Divider(context, ftk.Orientation.Vertical, layout)

        # The comparison options.
        self._compareComboBox = ftk.ComboBox(
            context, [tl.getLabel(mode) for mode in tl.getCompareEnums()])
        self._compareComboBox.hStretch = ftk.Stretch.Expanding

        self._wipeXSlider = ftk.FloatEditSlider(context)
        self._wipeXSlider.defaultValue = 0.5
        self._wipeYSlider = ftk.FloatEditSlider(context)
        self._wipeYSlider.defaultValue = 0.5
        self._wipeRotationSlider = ftk.FloatEditSlider(context)
        self._wipeRotationSlider.range = ftk.RangeF(0.0, 360.0)
        self._wipeRotationSlider.step = 1.0
        self._wipeRotationSlider.largeStep = 10.0
        self._wipeRotationSlider.defaultValue = 0.0

        self._overlaySlider = ftk.FloatEditSlider(context)
        self._overlaySlider.defaultValue = 0.5

        self._gainSlider = ftk.FloatEditSlider(context)
        # A gain is a multiplier with no natural bound.
        self._gainSlider.getModel().rangeSoft = True
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
        self._compareForm = form
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
        self._bellows = {"Compare": self._compareBellows}
        self._loadBellows(self._bellows)

        self._setContent(layout)

        appWeak = weakref.ref(app)
        self._fileList.setActivateCallback(
            lambda index: appWeak().getFilesModel().setA(index))
        self._fileList.setDeleteCallback(
            lambda index: appWeak().getFilesModel().close(index))
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

    def sizeHintEvent(self, event):
        super().sizeHintEvent(event)
        self._handle = event.style.getSizeRole(
            ftk.SizeRole.Handle, event.displayScale)

    def drawOverlayEvent(self, drawRect, event):
        super().drawOverlayEvent(drawRect, event)
        if self._dropTarget != -1:
            g = self._getDropGeom(self._dropTarget)
            if g is not None:
                event.render.drawRect(
                    g, event.style.getColorRole(ftk.ColorRole.Checked))

    def dragEnterEvent(self, event):
        if isinstance(event.data, djv.ui.FileDragDropData):
            event.accept = True
            self._dropTarget = self._dropIndex(event.pos, event.data)
            self.setDrawUpdate()

    def dragLeaveEvent(self, event):
        if isinstance(event.data, djv.ui.FileDragDropData):
            event.accept = True
            self._dropTarget = -1
            self.setDrawUpdate()

    def dragMoveEvent(self, event):
        if isinstance(event.data, djv.ui.FileDragDropData):
            event.accept = True
            dropTarget = self._dropIndex(event.pos, event.data)
            if dropTarget != self._dropTarget:
                self._dropTarget = dropTarget
                self.setDrawUpdate()

    def dropEvent(self, event):
        if isinstance(event.data, djv.ui.FileDragDropData):
            event.accept = True
            if self._dropTarget != -1:
                filesModel = self._app().getFilesModel()
                for i, f in enumerate(filesModel.files):
                    if f is event.data.item:
                        to = self._dropTarget
                        if i < to:
                            to -= 1
                        filesModel.move(i, to)
                        break
            self._dropTarget = -1
            self.setDrawUpdate()

    def _rowSpanY(self, row):
        g = self._rowWidgets[row][0].geometry
        return (g.min.y, g.max.y)

    def _dropIndex(self, pos, data):
        # The gaps around the dragged row itself are where it already is:
        # dropping there moves nothing, so nothing is shown.
        out = self._getDropIndex(pos)
        for i, w in enumerate(self._rowWidgets):
            if w[5] is data.item:
                if out == i or out == i + 1:
                    out = -1
                break
        return out

    def _getDropIndex(self, pos):
        # Only over the rows themselves, with a little reach: the tool also
        # holds the comparison section, and a drop there should mean
        # nothing.
        out = -1
        g = self._fileList.geometry
        m = self._handle
        if self._rowWidgets and \
                g.min.x - m <= pos.x <= g.max.x + m and \
                g.min.y - m <= pos.y <= g.max.y + m:
            out = 0
            for row in range(len(self._rowWidgets)):
                lo, hi = self._rowSpanY(row)
                if pos.y < (lo + hi) // 2:
                    break
                out += 1
        return out

    def _getDropGeom(self, index):
        # Centered in the gap between the rows; at the ends there is no
        # gap, so the row's own edge is the line.
        if not self._rowWidgets:
            return None
        count = len(self._rowWidgets)
        if 0 == index:
            y = self._rowSpanY(0)[0]
        elif index < count:
            y = (self._rowSpanY(index - 1)[1] + self._rowSpanY(index)[0]) // 2
        else:
            y = self._rowSpanY(count - 1)[1]
        g = self._fileList.geometry
        return ftk.Box2I(
            g.min.x, y - self._handle // 2, g.w, self._handle)

    def _filesUpdate(self, files):
        # The same files in a different order: move the rows rather than
        # rebuilding them. Rebuilding makes new thumbnails and lays out an
        # empty grid along the way, which loses the scroll position, so
        # reordering a long list would jump. The same files in the same
        # order fall through to the rebuild -- that is refresh(),
        # announcing that what the items hold has changed. The button
        # groups follow the new order, since they answer clicks with an
        # index in the order their buttons were added.
        if files and len(files) == len(self._rowWidgets):
            widgets = []
            reordered = False
            for i, item in enumerate(files):
                match = next(
                    (w for w in self._rowWidgets if w[5] is item), None)
                if match is None:
                    break
                widgets.append(match)
                reordered |= self._rowWidgets[i][5] is not item
            if len(widgets) == len(files) and reordered:
                self._rowWidgets = widgets
                self._bButtonGroup.clearButtons()
                for row, w in enumerate(self._rowWidgets):
                    self._bButtonGroup.addButton(w[1])
                    self._fileList.moveToIndex(w[0], row)
                return

        self._rowWidgets = []
        self._bButtonGroup.clearButtons()
        self._fileList.clear()
        app = self._app()
        a = app.getFilesModel().a
        b = app.getFilesModel().b
        seqExts = tl.getExts(app.context, int(tl.FileType.Seq))
        appWeak = self._app
        for row, item in enumerate(files):
            # The row is one item: click or Return sets "A", and a press
            # that moves drags the file to a new place in the list.
            button = ftk.ItemButton(self.context, self._fileList)
            button.acceptsKeyFocus = False
            button.checked = item is a
            button.tooltip = item.path.get() + "\n\nSet the A file."

            rowLayout = ftk.HorizontalLayout(self.context)
            rowLayout.spacingRole = ftk.SizeRole.SpacingSmall

            thumbnail = djv.ui.FileThumbnail(
                self.context, item,
                app.getSettingsModel().ioOptions, rowLayout)

            nameLabel = ftk.Label(
                self.context, ftk.elide(item.path.fileName, 24), rowLayout)
            nameLabel.hStretch = ftk.Stretch.Expanding
            nameLabel.vAlign = ftk.VAlign.Center

            def clicked(captured = item, appWeak = appWeak):
                filesModel = appWeak().getFilesModel()
                # By the item rather than a captured index: the rows move.
                for i, f in enumerate(filesModel.files):
                    if f is captured:
                        filesModel.setA(i)
                        break
            button.setClickedCallback(clicked)

            button.setDragDropDataCallback(
                lambda captured = item: djv.ui.FileDragDropData(captured))
            button.setDragDropCursorCallback(
                lambda thumbnailWeak = weakref.ref(thumbnail):
                    thumbnailWeak().thumbnail if thumbnailWeak() else None)

            layerComboBox = ftk.ComboBox(self.context, rowLayout)
            layerComboBox.setItems(item.videoLayers)
            layerComboBox.currentIndex = item.videoLayer
            layerComboBox.vAlign = ftk.VAlign.Center
            layerComboBox.tooltip = "Set the current layer."
            # Layer names can be long, and the column is as wide as the
            # longest one in it. Kept from the end: layer names share a
            # prefix and differ where they finish.
            layerComboBox.setElide(12, ftk.ElideMode.Left)
            layerComboBox.setVisible(len(item.videoLayers) > 1)
            layerComboBox.setIndexCallback(
                lambda value, captured = item, appWeak = self._app:
                    appWeak().getFilesModel().setLayer(captured, value))

            # Only an image sequence has a frame range to state. The range
            # is what the sequence is meant to cover, which need not be
            # what is on disk yet.
            if item.path.hasNum and item.path.testExt(seqExts):
                if item.timeRange is not None:
                    start = int(item.timeRange.start_time.value)
                    duration = int(item.timeRange.duration.value)
                    frames = ftk.RangeI64(start, start + duration - 1)
                elif item.path.frames is not None:
                    frames = item.path.frames
                else:
                    frames = ftk.RangeI64(0, 0)
                rangeButton = ftk.ToolButton(
                    self.context,
                    "{}-{}".format(frames.min, frames.max),
                    rowLayout)
                rangeButton.vAlign = ftk.VAlign.Center
                rangeButton.tooltip = "The frame range of the sequence."
                rangeButton.setClickedCallback(
                    lambda captured = item, r = frames, index = row, \
                        f = Util.weak(self._showRangePopup):
                        f(captured, r, index))
            else:
                rangeButton = None

            # Last, so the one control every row has lines up at the
            # right edge whatever else the row holds.
            bButton = ftk.ToolButton(self.context, "B", rowLayout)
            # Its own color, not the row's: a checked "B" on the checked
            # "A" row would vanish into it.
            bButton.checkedRole = ftk.ColorRole.Blue
            bButton.checked = any(item is i for i in b)
            bButton.vAlign = ftk.VAlign.Center
            bButton.tooltip = "Set the B file(s)."
            self._bButtonGroup.addButton(bButton)

            button.widget = rowLayout

            self._rowWidgets.append(
                (button, bButton, layerComboBox, rangeButton, thumbnail,
                 item))

    def _showRangePopup(self, item, frames, row):
        if row >= len(self._rowWidgets):
            return
        button = self._rowWidgets[row][3]
        if button is None:
            return
        # The button toggles, the way the slider popups do: the press
        # falls through the open popup to the button, and the click
        # closes it.
        if self._rangePopup:
            self._rangePopup.close()
            self._rangePopup = None
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
        for i, widgets in enumerate(self._rowWidgets):
            widgets[0].checked = i == value
        if 0 <= value < len(self._rowWidgets):
            # The keyboard starts from "A".
            self._fileList.current = value

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
        wipe = tl.Compare.Wipe == options.compare
        self._compareForm.setRowVisible(self._wipeXSlider, wipe)
        self._compareForm.setRowVisible(self._wipeYSlider, wipe)
        self._compareForm.setRowVisible(self._wipeRotationSlider, wipe)
        self._compareForm.setRowVisible(
            self._overlaySlider, tl.Compare.Overlay == options.compare)
        self._compareForm.setRowVisible(
            self._gainSlider, tl.Compare.Difference == options.compare)

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
        layout.spacingRole = ftk.SizeRole.Border
        self._bellows = {}
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
            if hasattr(widget, "enabledCheckBox"):
                bellows.toolWidget = widget.enabledCheckBox
            self._bellows[title] = bellows
        self._setContent(layout)
        self._loadBellows(self._bellows)

class ColorTool(IToolWidget):
    """
    This tool provides the color management and adjustments.
    """
    def __init__(self, context, app, mainWindow, parent = None):
        IToolWidget.__init__(self, context, app, mainWindow, "Color", "ColorTool", parent)

        colorModel = app.getColorModel()
        viewportModel = app.getViewportModel()
        layout = ftk.VerticalLayout(context)
        layout.spacingRole = ftk.SizeRole.Border
        self._bellows = {}
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
            self._bellows[title] = bellows
        self._setContent(layout)
        self._loadBellows(self._bellows)

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

        self._loadBellows(self._bellows)

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

class MagnifyTool(IToolWidget):
    """
    This tool magnifies the viewport at the sample position.
    """
    def __init__(self, context, app, mainWindow, parent = None):
        IToolWidget.__init__(
            self, context, app, mainWindow, "Magnify", "MagnifyTool", parent)

        self._widget = djv.ui.MagnifyWidget(
            context,
            app.settings,
            mainWindow.getViewport(),
            app.getFilesModel(),
            app.getColorModel(),
            app.getViewportModel(),
            app.getSettingsModel())
        # The magnified view has no natural size of its own, so this
        # takes what room is left rather than a band of its own.
        self.vStretch = ftk.Stretch.Expanding
        self._setContent(self._widget)

        selfWeak = weakref.ref(self)
        self._playerObserver = tl.PlayerObserver(
            app.observePlayer(),
            lambda player: selfWeak()._widget.setPlayer(player))

class ExportTool(IToolWidget):
    """
    This tool exports images, sequences, and movies.
    """
    def __init__(self, context, app, mainWindow, parent = None):
        IToolWidget.__init__(
            self, context, app, mainWindow, "Export", "ExportTool", parent)

        self._widget = djv.ui.ExportWidget(
            context,
            app.getFilesModel(),
            app.getColorModel(),
            app.getViewportModel(),
            app.getSettingsModel(),
            app.getTimeUnitsModel())
        self._setContent(self._widget)

        selfWeak = weakref.ref(self)
        self._playerObserver = tl.PlayerObserver(
            app.observePlayer(),
            lambda player: selfWeak()._widget.setPlayer(player))

class DiagTool(IToolWidget):
    """
    This tool displays diagnostics.
    """
    def __init__(self, context, app, mainWindow, parent = None):
        IToolWidget.__init__(
            self, context, app, mainWindow, "Diagnostics", "DiagTool", parent)

        widget = ftk.DiagWidget(context)
        widget.marginRole = ftk.SizeRole.Margin
        # The contents have no natural end, so this takes what room is
        # left rather than a band of its own while other tools sit at
        # the height they need.
        self.vStretch = ftk.Stretch.Expanding
        self._setContent(widget)

# The tools this application implements so far; the tools model lists
# more, and the actions only offer what can actually open.
def _formatCreated(iso):
    """
    Format an ISO 8601 timestamp for display in local time, e.g.
    "2026-07-26T17:22:06Z" -> "26/07/2026 - 19:22" at UTC+2. Falls back
    to the raw string if it is not the expected shape.
    """
    try:
        import calendar, time
        t = calendar.timegm(time.strptime(iso, "%Y-%m-%dT%H:%M:%SZ"))
        return time.strftime("%d/%m/%Y - %H:%M", time.localtime(t))
    except ValueError:
        return iso

def _isSingleFrame(range):
    return range.duration.value <= 1.0

def _formatRange(range, units):
    """
    Format a range in the current time units, so the labels read the
    same as the times on the timeline. A single frame is one value; a
    span is both ends, inclusive.
    """
    if _isSingleFrame(range):
        return tl.timeToText(range.start_time, units)
    # Frame numbers read as one token; the longer forms need the space.
    separator = "-" if tl.TimeUnits.Frames == units else " - "
    return (tl.timeToText(range.start_time, units) + separator +
        tl.timeToText(range.end_time_inclusive(), units))

def _markerTitle(marker, units):
    """
    The title a marker's row shows: the name where there is one, its
    frames otherwise, and what it is about failing both.
    """
    if marker.name:
        return marker.name
    if marker.range is not None:
        return _formatRange(marker.range, units)
    return "No frame"

def _markerShowing(marker, currentTime):
    """
    Whether the marker speaks about the frame being shown.
    """
    return (marker.range is not None and
        currentTime is not None and
        marker.range.contains(currentTime))

def _wrapText(text, columns):
    """
    Soft-wrap text to a column. ftk.Label renders newlines but does not
    wrap on its own, so long lines would otherwise overflow.
    """
    out = []
    for line in text.split("\n"):
        current = ""
        for word in line.split(" "):
            if current and len(current) + len(word) + 1 > columns:
                out.append(current)
                current = word
            elif current:
                current += " " + word
            else:
                current = word
        out.append(current)
    return "\n".join(out)

class ReviewTool(IToolWidget):
    """
    This tool provides the review drawing and markers.
    """
    def __init__(self, context, app, mainWindow, parent = None):
        IToolWidget.__init__(
            self, context, app, mainWindow, "Review", "ReviewTool", parent)

        drawModel = app.getDrawModel()
        self._player = None
        self._currentTime = None
        self._inOutRange = None
        self._markers = []
        self._markerButtons = {}
        self._swatches = {}
        self._currentTimeObserver = None
        self._inOutRangeObserver = None

        # Markers. The list is the bellows content with no margin, so
        # the items run edge to edge.
        # The add buttons live in the bellows title row, so the list
        # reads like the lists elsewhere: + in the header, - on the rows.
        self._addNoteButton = ftk.ToolButton(context)
        self._addNoteButton.icon = "Add"
        self._addRangeButton = ftk.ToolButton(context)
        self._addRangeButton.icon = "FrameInOut"
        # One delete for the list, acting on the focused row, rather
        # than one on every row. Clicking it must not move the key
        # focus, or it would clear the very selection it acts on.
        self._deleteButton = ftk.ToolButton(context)
        self._deleteButton.icon = "Remove"
        self._deleteButton.tooltip = "Delete the selected or active marker."
        self._deleteButton.enabled = False
        # The delete takes the key focus like any button. Taking it
        # puts down the row focus, and the target falls back to the
        # marker on the current frame -- the row's own Delete key is
        # the path that acts on a focused row exactly.
        markerToolLayout = ftk.HorizontalLayout(context)
        markerToolLayout.spacingRole = ftk.SizeRole._None
        self._addNoteButton.parent = markerToolLayout
        self._addRangeButton.parent = markerToolLayout
        self._deleteButton.parent = markerToolLayout
        # The list is the focus unit: one tab stop, a current item the
        # arrows move, and rows that do not take the focus themselves.
        self._markerListLayout = ftk.ItemButtonList(context)
        self._markerListLayout.spacingRole = ftk.SizeRole._None
        self._markerListLayout.setCurrentCallback(
            Util.weak(self._currentItem))
        self._markerListLayout.setActivateCallback(
            Util.weak(self._activateItem))
        self._markerListLayout.setDeleteCallback(
            Util.weak(self._deleteItem))

        # Drawing.
        drawingWidget = ftk.VerticalLayout(context)
        drawingWidget.marginRole = ftk.SizeRole.MarginSmall
        drawingWidget.spacingRole = ftk.SizeRole.SpacingSmall
        toolLayout = ftk.HorizontalLayout(context, drawingWidget)
        toolLayout.spacingRole = ftk.SizeRole.SpacingSmall
        self._colorSwatch = ftk.ColorSwatch(context, toolLayout)
        self._colorSwatch.editable = True
        # Centered rather than filling the row: the buttons beside it
        # set the row height, and a filled swatch reads as a rectangle.
        self._colorSwatch.vAlign = ftk.VAlign.Center
        self._colorSwatch.sizeRole = ftk.SizeRole.MarginLarge
        self._colorSwatch.color = drawModel.color
        self._colorSwatch.tooltip = "The stroke colour."
        self._penButton = ftk.ToolButton(context, toolLayout)
        self._penButton.icon = "DrawTool"
        # Deliberately not checkable: a checkable button flips its own
        # state after the callback, which would invert whatever the
        # model observer had just set. The model stays the only source
        # of truth and the observer drives the highlight.
        self._eraserButton = ftk.ToolButton(context, toolLayout)
        self._eraserButton.icon = "Eraser"
        toolLayout.addSpacer(ftk.SizeRole._None, ftk.Stretch.Expanding)
        self._undoButton = ftk.ToolButton(context, toolLayout)
        self._undoButton.icon = "Undo"
        self._redoButton = ftk.ToolButton(context, toolLayout)
        self._redoButton.icon = "Redo"
        self._clearDrawingButton = ftk.ToolButton(context, "Clear", toolLayout)
        sizeLayout = ftk.HorizontalLayout(context, drawingWidget)
        sizeLayout.spacingRole = ftk.SizeRole.SpacingSmall
        sizeLabel = ftk.Label(context, "Size:", sizeLayout)
        sizeLabel.vAlign = ftk.VAlign.Center
        self._sizeSlider = ftk.FloatEditSlider(context, sizeLayout)
        self._sizeSlider.setRange(1.0, 50.0)
        self._sizeSlider.value = drawModel.size
        self._sizeSlider.tooltip = "The stroke width, in source pixels."

        self._itemOrder = []

        # The tooltips come from the actions, where the shortcut
        # bindings are appended (#839). The buttons stay unbound
        # otherwise: see the pen button above for why.
        reviewActions = mainWindow.getReviewActions().actions
        self._tooltipObservers = []
        def bindTooltip(widget, action):
            def update(value, widget = widget):
                widget.tooltip = value
            self._tooltipObservers.append(
                ftk.StringObserver(action.observeTooltip, update))
        bindTooltip(self._addNoteButton, reviewActions["AddNote"])
        bindTooltip(self._addRangeButton, reviewActions["AddRange"])
        bindTooltip(self._penButton, reviewActions["Draw"])
        bindTooltip(self._eraserButton, reviewActions["Erase"])
        bindTooltip(self._undoButton, reviewActions["Undo"])
        bindTooltip(self._redoButton, reviewActions["Redo"])
        bindTooltip(self._clearDrawingButton, reviewActions["ClearDrawing"])

        # The marker being edited in place, a new marker being written,
        # and the deferred work both need: the commit is deferred a tick
        # because the focus loss is reported from inside the editor and
        # the commit rebuilds the list that owns it, and the scroll is
        # deferred a tick because a rebuilt list has no geometry yet.
        self._editingId = None
        self._editEdit = None
        # Before any observer can rebuild the list: the labels read this.
        self._timeUnits = app.getTimeUnitsModel().timeUnits
        self._editItem = None
        self._commitTimer = ftk.Timer(context)
        self._scrollTimer = ftk.Timer(context)

        layout = ftk.VerticalLayout(context)
        layout.spacingRole = ftk.SizeRole.Border
        self._scrollLayout = layout
        self._bellows = {}
        for title, widget, toolWidget in [
            ("Drawing", drawingWidget, None),
            ("Markers", self._markerListLayout, markerToolLayout),
        ]:
            bellows = ftk.Bellows(context, title, layout)
            bellows.widget = widget
            if toolWidget is not None:
                bellows.toolWidget = toolWidget
            bellows.open = True
            self._bellows[title] = bellows

        self._scrollWidget = ftk.ScrollWidget(context)
        self._scrollWidget.border = False
        self._scrollWidget.widget = layout
        # The markers have no natural end, so take what room is left
        # rather than a band of its own while other tools sit at the
        # height they need. The scroll widget has to expand with the
        # tool, or the extra room stays empty below it.
        self._scrollWidget.vStretch = ftk.Stretch.Expanding
        self.vStretch = ftk.Stretch.Expanding
        self._setContent(self._scrollWidget)

        appWeak = weakref.ref(app)
        selfWeak = weakref.ref(self)

        self._colorSwatch.setCallback(
            lambda value: appWeak() and
                appWeak().getDrawModel().setColor(value))

        # Pen and eraser are the only way in and out of drawing:
        # selecting one turns drawing on, clicking the active one turns
        # it off and gives the left mouse button back to the frame
        # shuttle.
        def toolClicked(tool):
            app_ = appWeak()
            if app_ is None:
                return
            drawModel = app_.getDrawModel()
            active = drawModel.enabled and drawModel.tool == tool
            drawModel.tool = tool
            drawModel.enabled = not active
        self._penButton.setClickedCallback(
            lambda: toolClicked(djv.models.DrawTool.Pen))
        self._eraserButton.setClickedCallback(
            lambda: toolClicked(djv.models.DrawTool.Eraser))

        self._sizeSlider.setCallback(
            lambda value: appWeak() and
                setattr(appWeak().getDrawModel(), "size", value))
        self._undoButton.setClickedCallback(
            lambda: appWeak() and appWeak().getAnnotationsModel().undo())
        self._redoButton.setClickedCallback(
            lambda: appWeak() and appWeak().getAnnotationsModel().redo())

        def clearFrame():
            app_ = appWeak()
            if app_ is None:
                return
            a = app_.getFilesModel().a
            player = app_.observePlayer().get()
            if a and player:
                app_.getAnnotationsModel().clearFrame(a.id, player.currentTime)
        self._clearDrawingButton.setClickedCallback(clearFrame)

        self._toolObserver = djv.models.DrawToolObserver(
            drawModel.observeTool,
            lambda value: selfWeak() and selfWeak()._drawStateUpdate())
        self._enabledObserver = ftk.BoolObserver(
            drawModel.observeEnabled,
            lambda value: selfWeak() and selfWeak()._drawStateUpdate())
        self._colorObserver = djv.models.Color4FObserver(
            drawModel.observeColor,
            lambda value: selfWeak() and setattr(
                selfWeak()._colorSwatch, "color", value))
        self._sizeObserver = ftk.FloatObserver(
            drawModel.observeSize,
            lambda value: selfWeak() and setattr(
                selfWeak()._sizeSlider, "value", value))
        self._hasUndoObserver = ftk.BoolObserver(
            app.getAnnotationsModel().observeHasUndo,
            lambda value: selfWeak() and setattr(
                selfWeak()._undoButton, "enabled", value))
        self._hasRedoObserver = ftk.BoolObserver(
            app.getAnnotationsModel().observeHasRedo,
            lambda value: selfWeak() and setattr(
                selfWeak()._redoButton, "enabled", value))

        self._addNoteButton.setClickedCallback(Util.weak(self.addNote))
        self._addRangeButton.setClickedCallback(Util.weak(self.addRange))
        self._deleteButton.setClickedCallback(Util.weak(self._deleteMarker))
        self._markersObserver = djv.models.ReviewMarkerListObserver(
            app.getMarkersModel().observeMarkers,
            lambda value: selfWeak() and selfWeak()._markersListUpdate(value))

        # The labels show times, so they follow the units the timeline
        # shows.
        self._timeUnitsObserver = tl.TimeUnitsObserver(
            app.getTimeUnitsModel().observeTimeUnits,
            lambda value: selfWeak() and selfWeak()._timeUnitsUpdate(value))

        # The list shows every marker; the playhead moves the highlight.
        self._playerObserver = tl.PlayerObserver(
            app.observePlayer(),
            lambda player: selfWeak() and selfWeak()._setPlayer(player))

        self._loadBellows(self._bellows)

    def addNote(self):
        """
        Add a marker about the current frame, edited in place in the
        list.
        """
        app = self._app()
        if app is None:
            return
        player = app.observePlayer().get()
        if player is None:
            return
        # Keep whatever was being written before starting the next
        # marker.
        self._commitMarker()
        # The marker exists from the moment it is asked for -- a bare
        # frame marker is a flag, the way a bare span is -- and its
        # text is edited in place.
        time = player.currentTime
        id = app.getMarkersModel().add(
            otio.opentime.TimeRange(
                time, otio.opentime.RationalTime(1.0, time.rate)),
            "", "")
        self._editMarker(id)

    def _timeUnitsUpdate(self, value):
        self._timeUnits = value
        self._markersUpdate()

    def _editMarker(self, id):
        if id == self._editingId:
            return
        self._commitMarker()
        self._editingId = id
        self._markersUpdate()

    def _commitMarker(self):
        if self._editEdit is None:
            return
        text = self._editEdit.text
        if isinstance(text, list):
            text = "\n".join(text)
        id = self._editingId
        # Clear the state before touching the model: the model observer
        # rebuilds the list, and must not find a half-finished edit.
        self._editingId = None
        self._editEdit = None
        # Before any observer can rebuild the list: the labels read this.
        self._timeUnits = app.getTimeUnitsModel().timeUnits
        self._editItem = None
        app = self._app()
        if app is not None and id is not None:
            # Emptied text is kept as emptied: a bare marker is a flag,
            # not a mistake.
            app.getMarkersModel().update(id, text)
        # An unchanged edit does not move the model, so rebuild by
        # hand; the extra rebuild after a model change is harmless.
        self._markersUpdate()

    def _editFocus(self, editor, value):
        if editor is None or editor is not self._editEdit:
            return
        if value:
            # Focus came back before the deferred commit fired.
            self._commitTimer.stop()
            return
        selfWeak = weakref.ref(self)
        editorRef = weakref.ref(editor)
        def commit():
            widget = selfWeak()
            if widget is not None and \
                    editorRef() is widget._editEdit:
                widget._commitMarker()
        self._commitTimer.start(0.0, commit)

    def _currentItem(self, value):
        # Browsing the feedback is looking at the frames it is about,
        # so the arrows follow.
        if 0 <= value < len(self._itemOrder):
            marker = next(
                (m for m in self._markers
                    if m.id == self._itemOrder[value][1]), None)
            if marker is not None and marker.range is not None:
                self._goToRange(marker.range)
        self._deleteButtonUpdate()

    def _activateItem(self, value):
        if 0 <= value < len(self._itemOrder):
            self._markerClicked(self._itemOrder[value][1])

    def _deleteItem(self, value):
        if 0 <= value < len(self._itemOrder):
            self._deleteMarkerId(self._itemOrder[value][1], value)

    def _deleteTarget(self):
        # The list's current item first: it outlives the focus moving
        # to the delete button itself. Failing that, the marker on the
        # current frame.
        current = self._markerListLayout.current
        if 0 <= current < len(self._itemOrder):
            return self._itemOrder[current][1]
        for button, id in self._itemOrder:
            if button.checked:
                return id
        return None

    def _deleteButtonUpdate(self):
        self._deleteButton.enabled = self._deleteTarget() is not None

    def _deleteMarker(self):
        id = self._deleteTarget()
        if id is None:
            return
        index = next(
            (i for i, (b, rowId) in enumerate(self._itemOrder)
                if rowId == id), 0)
        self._deleteMarkerId(id, index)

    def _deleteMarkerId(self, id, index):
        app = self._app()
        if app is not None:
            app.getMarkersModel().remove(id)
        # The current item lands on the neighbour, without following
        # its frames: repeated deletes cull the list without browsing.
        if self._itemOrder:
            self._markerListLayout.current = min(
                index, len(self._itemOrder) - 1)
        self._deleteButtonUpdate()

    def _seekTo(self, time):
        if not self._player:
            return
        # Going to feedback wins over a narrower in/out range: with the
        # target outside it, the seek would move the clock into a span
        # the player cannot show.
        if not self._player.inOutRange.contains(time):
            self._player.resetInPoint()
            self._player.resetOutPoint()
        self._player.currentTime = time

    def _markerClicked(self, id):
        if id is None or id == self._editingId:
            return
        marker = next((m for m in self._markers if m.id == id), None)
        if marker is None:
            return
        for j, (b, rowId) in enumerate(self._itemOrder):
            if rowId == id:
                self._markerListLayout.current = j
                break
        # The first click goes to the marker's frames -- a span narrows
        # the timeline to itself on the way; a click on the marker
        # already showing -- or on one about no frame in particular --
        # opens it for editing.
        if marker.range is None or _markerShowing(marker, self._currentTime):
            self._editMarker(id)
        elif self._player:
            self._goToRange(marker.range)
            # The keyboard continues from here.
            self._markerListLayout.takeKeyFocus()
        self._deleteButtonUpdate()

    def _goToRange(self, range_):
        if not self._player:
            return
        if _isSingleFrame(range_):
            self._seekTo(range_.start_time)
        else:
            # Going to a span narrows the timeline to it: the span is
            # what the marker is about, and the in/out points are how
            # the timeline says "these frames". Going elsewhere is what
            # gives the timeline back.
            self._player.inOutRange = range_
            self._player.currentTime = range_.start_time

    def _setPlayer(self, player):
        self._player = player
        # A marker is anchored to the current frame, so without media
        # there is nothing to attach it to.
        self._addNoteButton.enabled = player is not None
        selfWeak = weakref.ref(self)
        if player:
            self._currentTimeObserver = tl.RationalTimeObserver(
                player.observeCurrentTime,
                lambda value: selfWeak() and
                    selfWeak()._currentTimeUpdate(value))
            self._inOutRangeObserver = tl.TimeRangeObserver(
                player.observeInOutRange,
                lambda value: selfWeak() and selfWeak()._inOutUpdate(value))
        else:
            self._currentTimeObserver = None
            self._inOutRangeObserver = None
            self._currentTime = None
            self._inOutRange = None
            self._selectionUpdate()
            self._inOutStateUpdate()

    def _currentTimeUpdate(self, value):
        self._currentTime = value
        self._selectionUpdate()

    def _inOutUpdate(self, value):
        self._inOutRange = value
        self._inOutStateUpdate()

    def _drawStateUpdate(self):
        if self._app():
            drawModel = self._app().getDrawModel()
            enabled = drawModel.enabled
            tool = drawModel.tool
            self._penButton.checked = \
                enabled and djv.models.DrawTool.Pen == tool
            self._eraserButton.checked = \
                enabled and djv.models.DrawTool.Eraser == tool

    def _inOutStateUpdate(self):
        # Adding is only meaningful once the in/out points actually
        # narrow the timeline.
        narrowed = (
            self._player is not None and
            self._inOutRange is not None and
            self._inOutRange != self._player.timeRange)
        self._addRangeButton.enabled = narrowed

    def addRange(self):
        """
        Add a marker for the timeline in/out points, edited in place.
        """
        if not self._player or self._inOutRange is None:
            return
        # No dialog and no name: the marker's row is titled by its
        # frames, and words belong in its text -- the editor opens in
        # place, and leaving it empty leaves the bare span. The two add
        # gestures differ only in the span they stamp.
        app = self._app()
        if app is not None:
            self._commitMarker()
            id = app.getMarkersModel().add(self._inOutRange, "", "")
            self._editMarker(id)

    def _markersListUpdate(self, markers):
        # A color-only change updates the rows in place: rebuilding
        # would destroy the very swatch whose popup the user is still
        # picking from.
        def sameButColor(a, b):
            return (a.id == b.id and a.name == b.name and
                djv.models.sameRange(a.range, b.range) and
                a.text == b.text and a.author == b.author and
                a.created == b.created)
        colorOnly = (len(self._markers) > 0 and
            len(markers) == len(self._markers) and
            all(sameButColor(a, b)
                for a, b in zip(markers, self._markers)))
        self._markers = markers
        if colorOnly:
            for marker in markers:
                swatch = self._swatches.get(marker.id)
                if swatch is not None:
                    swatch.color = marker.color
        else:
            self._markersUpdate()

    def _markersUpdate(self):
        # The current item survives the rebuild by identifier.
        currentId = None
        current = self._markerListLayout.current
        if 0 <= current < len(self._itemOrder):
            currentId = self._itemOrder[current][1]
        # Let go of any editor before the clear destroys it, so the
        # focus loss it reports on the way out finds nothing left to
        # commit.
        self._editEdit = None
        self._editItem = None
        self._markerListLayout.clear()
        self._markerButtons = {}
        self._swatches = {}
        self._itemOrder = []
        context = self.context
        # Every marker, so the panel reads as the review's feedback
        # rather than one frame's -- browsing beats following bread
        # crumbs. The model keeps the list in time order, with the
        # markers about no frame in particular first: they speak about
        # the whole review.
        value = self._markers
        if not value:
            # Without this the section is silently empty, which reads
            # as a bug rather than as "nothing to say yet".
            label = ftk.Label(
                context, "No markers yet.", self._markerListLayout)
            label.marginRole = ftk.SizeRole.MarginSmall
            label.textRole = ftk.ColorRole.TextDisabled
            self._deleteButtonUpdate()
            return
        appWeak = self._app
        selfWeak = weakref.ref(self)
        first = True
        for marker in value:
            if not first:
                ftk.Divider(
                    context, ftk.Orientation.Vertical, self._markerListLayout)
            first = False
            # The whole marker is one item button: the marker is what
            # the click selects, not one widget inside it.
            button = ftk.ItemButton(context, self._markerListLayout)
            hasRange = marker.range is not None
            editing = (marker.id is not None and
                marker.id == self._editingId)
            if not editing:
                if not hasRange:
                    button.tooltip = "Edit the marker."
                elif _isSingleFrame(marker.range):
                    button.tooltip = (
                        "Go to the marker's frame.\n\nClick the marker "
                        "already showing to edit it.")
                else:
                    button.tooltip = (
                        "Go to the marker's frames, setting the "
                        "timeline in/out points to them.\n\nClick the "
                        "marker already showing to edit it.")
            button.setClickedCallback(
                lambda captured = marker.id:
                    selfWeak() and selfWeak()._markerClicked(captured))
            if marker.id is not None:
                self._markerButtons[marker.id] = button
                self._itemOrder.append((button, marker.id))
            # The list is the focus unit; the rows are not tab stops.
            button.acceptsKeyFocus = False
            # One margin around the whole marker, carried by the card,
            # so the header and the text sit evenly inside the item.
            card = ftk.VerticalLayout(context)
            card.marginRole = ftk.SizeRole.MarginInside
            card.spacingRole = ftk.SizeRole.Spacing
            button.widget = card
            header = ftk.HorizontalLayout(context, card)
            header.spacingRole = ftk.SizeRole.SpacingSmall
            # The marker's color, editable in place as a small dot --
            # quieter than a swatch in a list of many. The dot takes
            # the click itself, so choosing a color does not read as
            # clicking the row.
            if marker.id is not None:
                swatch = ftk.ColorDot(context, header)
                swatch.vAlign = ftk.VAlign.Center
                self._swatches[marker.id] = swatch
                swatch.color = marker.color
                swatch.editable = True
                # The row is the focus unit in this list; the dot stays
                # out of the tab order. After editable, which opts in.
                swatch.acceptsKeyFocus = False
                swatch.tooltip = "The marker's color."
                def colorPicked(value, captured = marker.id,
                        appWeak = appWeak):
                    app_ = appWeak()
                    if app_ is not None:
                        app_.getMarkersModel().updateColor(captured, value)
                swatch.setCallback(colorPicked)
            titleLabel = ftk.Label(
                context, _markerTitle(marker, self._timeUnits), header)
            titleLabel.setMarginRole(ftk.SizeRole.LabelPad, ftk.SizeRole._None)
            titleLabel.vAlign = ftk.VAlign.Center
            titleLabel.hStretch = ftk.Stretch.Expanding
            # The frames sit against the right edge, the way the file
            # browser lays out its columns, so a named row says where
            # it points -- unless the title is the frames already, and
            # saying them twice reads as a mistake.
            if hasRange and marker.name:
                frames = _formatRange(marker.range, self._timeUnits)
                if marker.name != frames:
                    framesLabel = ftk.Label(context, frames, header)
                    framesLabel.setMarginRole(
                        ftk.SizeRole.LabelPad, ftk.SizeRole._None)
                    framesLabel.textRole = ftk.ColorRole.TextDisabled
                    framesLabel.vAlign = ftk.VAlign.Center
            if marker.created:
                createdLabel = ftk.Label(
                    context, _formatCreated(marker.created), header)
                createdLabel.setMarginRole(
                    ftk.SizeRole.LabelPad, ftk.SizeRole._None)
                createdLabel.textRole = ftk.ColorRole.TextDisabled
                createdLabel.vAlign = ftk.VAlign.Center
            if editing:
                # Written and edited in place. The marker keeps itself
                # when the editor loses the focus, or on Command-Return.
                self._editEdit = ftk.TextEdit(context, card)
                self._editEdit.sizeHintRole = \
                    ftk.SizeRole.ScrollAreaSmall
                self._editEdit.setText(marker.text.split("\n"))
                editorRef = weakref.ref(self._editEdit)
                self._editEdit.setFocusCallback(
                    lambda value, editorRef = editorRef:
                        selfWeak() and
                        selfWeak()._editFocus(editorRef(), value))
                self._editItem = button
                self._scrollToEdit()
            elif marker.text:
                textLabel = ftk.Label(
                    context, _wrapText(marker.text, 40), card)
                textLabel.setMarginRole(
                    ftk.SizeRole.LabelPad, ftk.SizeRole._None)
                textLabel.hAlign = ftk.HAlign.Left
                textLabel.vAlign = ftk.VAlign.Top
        for i, (b, rowId) in enumerate(self._itemOrder):
            if rowId == currentId:
                self._markerListLayout.current = i
                break
        if self._editEdit is not None:
            self._editEdit.takeKeyFocus()
        self._selectionUpdate()

    def _scrollToEdit(self):
        # Deferred a tick: only after a layout pass does the rebuilt
        # item have a geometry to scroll to.
        selfWeak = weakref.ref(self)
        def scroll():
            widget = selfWeak()
            if widget is None or widget._editItem is None:
                return
            g = widget._editItem.geometry
            c = widget._scrollLayout.geometry
            widget._scrollWidget.scrollTo(ftk.Box2I(
                g.min.x - c.min.x, g.min.y - c.min.y, g.w, g.h))
        self._scrollTimer.start(0.0, scroll)

    def _selectionUpdate(self):
        # Highlight the markers about the frame being shown.
        for marker in self._markers:
            button = self._markerButtons.get(marker.id)
            if button is not None:
                button.checked = _markerShowing(marker, self._currentTime)
        self._deleteButtonUpdate()

FACTORY = {
    "Files": FilesTool,
    "Color Picker": ColorPickerTool,
    "Magnify": MagnifyTool,
    "Diagnostics": DiagTool,
    "Export": ExportTool,
    "View": ViewTool,
    "Color": ColorTool,
    "Information": InfoTool,
    "Audio": AudioTool,
    "Review": ReviewTool,
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
        self._displayed = True

        self._layout = ftk.VerticalLayout(context)
        self._layout.spacingRole = ftk.SizeRole.SpacingSmall

        # One scroll area for the whole stack rather than one inside
        # each tool, so a tool takes the height its contents need.
        self._scrollWidget = ftk.ScrollWidget(context, ftk.ScrollType.Both)
        self._scrollWidget.border = False
        # Reordering the file list by dragging needs the list to move when
        # the drag reaches the edge of the panel.
        self._scrollWidget.dragScroll = True
        self._scrollWidget.widget = self._layout
        self._setWidget(self._scrollWidget)

        selfWeak = weakref.ref(self)
        self._openObserver = ftk.StringListObserver(
            app.getToolsModel().observeOpenTools,
            lambda names: selfWeak()._openUpdate(names))

    def getToolWidget(self, name):
        return self._widgets.get(name)

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
        self._visibleUpdate()

    def setDisplayed(self, value):
        """
        Set whether the window shows the panel; open tools stay open
        while it is hidden.
        """
        self._displayed = value
        self._visibleUpdate()

    def _visibleUpdate(self):
        self.setVisible(self._displayed and len(self._widgets) > 0)
