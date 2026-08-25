# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the DJV project.

import opentimelineio as otio
import ftkPy as ftk
import tlRenderPy as tl
import djvPy as djv

import json
import weakref

class IActions:
    """
    Base class for the action groups, following the C++ IActions: every
    operation is registered as a named command with the commands model,
    the actions run the commands, and the keyboard shortcuts come from
    the settings so that the shortcuts settings apply here too.
    """
    def __init__(self, context, app, name):

        self._name = name
        self.actions = {}
        self._tooltips = {}
        # Kept so that __del__ can remove the commands; the weakref to
        # the app is dead by then.
        self._settingsModel = app.getSettingsModel()
        self._commandsModel = app.getCommandsModel()
        self._commands = []

        selfWeak = weakref.ref(self)
        self._shortcutsSettingsObserver = djv.models.ShortcutsSettingsObserver(
            self._settingsModel.observeShortcuts,
            lambda value: selfWeak()._shortcutsUpdate(value))

    def __del__(self):
        for command in self._commands:
            self._commandsModel.remove(command)

    def _addCommand(self, name, doc, func):
        """
        Register a command named "<group>/<name>". The command function
        receives the JSON arguments as a string and may ignore them.
        The documentation is also used as the action tooltip.
        """
        commandName = "{}/{}".format(self._name, name)
        self._commandsModel.add(commandName, doc, func)
        self._commands.append(commandName)
        self._tooltips[name] = doc

    def _addCheckCommand(self, name, doc, func):
        """
        Register a command like _addCommand() whose function receives a
        checked state, parsed from { "value": <bool> }. Run by name and
        without arguments the state defaults to True: asking for a mode
        means turning it on.
        """
        commandDoc = doc
        if commandDoc.endswith("."):
            commandDoc = commandDoc[:-1]
        commandDoc += "; e.g., { \"value\": true }."
        def parse(args, f = func):
            parsed = json.loads(args) if args else None
            value = parsed.get("value", True) if isinstance(parsed, dict) else True
            f(value)
        commandName = "{}/{}".format(self._name, name)
        self._commandsModel.add(commandName, commandDoc, parse)
        self._commands.append(commandName)
        self._tooltips[name] = doc

    def _command(self, name):
        """
        Get a callback that executes the command named "<group>/<name>",
        suitable for use as an action callback.
        """
        commandName = "{}/{}".format(self._name, name)
        weak = weakref.ref(self._commandsModel)
        def call():
            model = weak()
            if model is not None:
                model.exec(commandName)
        return call

    def _checkCommand(self, name):
        """
        Get a callback that executes the command named "<group>/<name>"
        with the checked state as { "value": <bool> }, suitable for use
        as a checkable action callback.
        """
        commandName = "{}/{}".format(self._name, name)
        weak = weakref.ref(self._commandsModel)
        def call(value):
            model = weak()
            if model is not None:
                model.exec(commandName, json.dumps({ "value": value }))
        return call

    def _addShortcut(self, name, *args):
        """
        Register a keyboard shortcut named "<group>/<name>" with the
        settings model, listed under the action's text, or a label of
        its own when the first argument is a string. Any saved key
        binding overrides the given default.
        """
        if args and isinstance(args[0], str):
            label = args[0]
            args = args[1:]
        else:
            label = self.actions[name].text
        primary = args[0] if len(args) > 0 else ftk.KeyShortcut()
        secondary = args[1] if len(args) > 1 else ftk.KeyShortcut()
        if isinstance(primary, ftk.Key):
            primary = ftk.KeyShortcut(primary)
        self._settingsModel.addShortcuts([djv.models.Shortcut(
            "{}/{}".format(self._name, name),
            label,
            primary,
            secondary)])

    def _shortcutsUpdate(self, settings):
        for name, action in self.actions.items():
            fullName = "{}/{}".format(self._name, name)
            for shortcut in settings.shortcuts:
                if shortcut.name == fullName:
                    action.shortcuts = [shortcut.primary, shortcut.secondary]
                    tooltip = self._tooltips.get(name)
                    if tooltip is not None:
                        labels = []
                        for key in (shortcut.primary, shortcut.secondary):
                            if key.key != ftk.Key.Unknown:
                                labels.append(
                                    ftk.getShortcutLabel(key.key, key.modifiers))
                        if labels:
                            action.tooltip = "{}\n\nShortcut: {}".format(
                                tooltip, ", ".join(labels))
                        else:
                            action.tooltip = tooltip
                    break
