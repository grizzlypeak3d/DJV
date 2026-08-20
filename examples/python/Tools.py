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

# The tools this application implements so far; the tools model lists
# more, and the actions only offer what can actually open.
FACTORY = {
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
