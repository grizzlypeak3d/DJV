# DJV for agents

Instructions for a coding agent working on DJV or writing an extension for
it. People are welcome to read it too.

DJV is a playback and review application for film, VFX and animation. It is
C++ (CMake, C++20) on top of two libraries, built here as submodules:

- `deps/tlRender`: timelines, players and file I/O, with OpenTimelineIO.
  It has its own AGENTS.md.
- `deps/tlRender/deps/ftk`: feather-tk, the user interface toolkit.

## Where things are

- `lib/djv/Models`: the application's behavior, with no widgets: files,
  settings, color, audio, the viewport options, annotations, reviews, the
  HUD, the command registry. The desktop and the iPad both use these, so
  behavior goes here rather than in a widget.
- `lib/djv/UI`: widgets (the viewport, the tools).
- `lib/djv/App`: the application, its menus (`*Actions.cpp`) and tools.
- `lib/djv/C`: a C interface over the models, for the iPad.
- `lib/djv/*Py`, `lib/djv/djvPy`: the Python bindings (nanobind).
- `examples/python`: the Python example, which CI runs.
- `tests`: `djv-test` (C++), `djv-c-test` (C), `djvPyTest` (Python), and
  screenshot captures of the running application (`tests/capture`).
- `docs`: the HTML documentation. `command-line.html` is generated from the
  application's `-helpJSON`; do not edit it by hand.

## Building and testing

The super build builds the dependencies and then DJV:

```
sh DJV/sbuild-macos.sh      # or sbuild-linux.sh, sbuild-win.bat
```

Options go in `etc/Config/local.cmake` (not committed). For the tests:

```
set(DJV_TESTS ON CACHE BOOL "")
```

The captures also need the `djv` program (`DJV_PROGRAMS`, on by default).

and for Python, `set(DJV_PYTHON ON CACHE BOOL "")` (Python 3.12 or newer).
Then, in the build directory:

```
ctest -j6
```

`tl-test` and `djv-test` take a filter: `djv-test SettingsModelTest`. A test
build that was not rebuilt runs old binaries, so check that the test
executables are newer than your change.

## Extending DJV in Python

The application is the C++ one, bound as `djv.app.App`. A Python program
derives from it and overrides the steps it is built in, calling the base
first: `_modelsInit`, `_observersInit`, `_inputFilesInit`, `_uiInit`,
`_mainWindowInit`, in that order. The menus, tool bars, playback and models
stay the application's own, so an extension cannot drift from them.

`examples/python` is the reference, about 150 lines in three files:
`main.py` starts the application, `App.py` adds a tool and a command, and
`NotesTool.py` is the tool. It runs in CI as the `djv-python-example`
capture, so it is known to work; copy from it.

A tool is registered in `_uiInit`, with the tools model and the tool widget
factory, before the menus are built:

```python
class App(djv.app.App):
    def _uiInit(self):
        djv.app.App._uiInit(self)
        info = djv.models.ToolInfo()
        info.name = "Notes"
        info.icon = "Info"
        info.sort = "Notes"
        info.toolBar = True
        self.getToolsModel().addTool(info)
        self.getToolWidgetFactory().addTool("Notes", NotesTool)
```

The tool derives from `djv.app.IToolWidget` and sets its content with
`setWidget`; the frame, title and close button are the application's. It
updates itself by observing a model (`tl.PlayerObserver(app.observePlayer,
...)`), not from its own callbacks.

A command is added in `_mainWindowInit`. Every menu item is a named command,
so a command added here can be run from the command line
(`-command "Notes/Clear"`) and listed with `-listCommands`:

```python
    def _mainWindowInit(self):
        djv.app.App._mainWindowInit(self)
        self.getCommandsModel().add(
            "Notes/Clear", "Clear the notes.", weak(self._clearNotes))
```

Rules that matter in Python:

- Pass callbacks through a weak reference (`weak()` in `App.py`). A bound
  method held by C++ forms a cycle Python cannot collect, and the settings
  are written when the objects are destroyed.
- Keep a reference to every observer and action you create, or it is
  collected and stops working.
- Use `-settingsFile` and `-resetSettings` when running tests, so a test
  never touches the real settings.

## Changing DJV itself

- Every settings key is a constant in `lib/djv/Models/SettingsKeys.h`; a
  test checks that no key lies under another.
- Default keyboard shortcuts are in one table, `models::getDefaultShortcuts()`.
- New behavior goes in the models with a test in `lib/djv/ModelsTest`, and
  the widget or the iPad uses it from there.
- A new menu item is a command first (`*Actions.cpp`); the menu, tool bar,
  command line and scripts all reach it that way.
- `ChangeLog.md` lists user-visible changes, one short line each.

## Contributions

- Tests with every change, and the existing tests passing.
- One change per pull request.
- No new dependencies without discussing them first.
- The person who sends a change stands behind it, however it was written.
