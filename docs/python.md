# Python

The libraries DJV is built from have Python bindings, and there is a version of the DJV application written in Python on top of them. This page describes what is there, how to run it, and how to modify it. It is not an API reference — the bindings follow the C++ headers closely, so the headers and the Python examples are the reference.

## The modules

Four modules make up the stack:

- **feather_tk** — the [feather-tk](https://github.com/darbyjohnston/feather-tk) user interface toolkit: widgets, layouts, observables, and the application classes.
- **tlrender** — timelines, players, and file I/O, with the timeline and viewport widgets in the `tl.ui` submodule.
- **djv** — DJV itself: the data models in the `djv.models` submodule — files, settings, color, audio, viewport, and tools — and the application's widgets in the `djv.ui` submodule — the viewport with its HUD and color picking, and the color, view, settings, export, and magnify widgets.
- **opentimelineio** — the standard OTIO package. Its time types pass directly through the tlRender bindings, so a `RationalTime` from OTIO seeks a player.

The conventional imports:

```
import opentimelineio as otio
import feather_tk as ftk
import tlrender as tl
import djv
```

## Building

One switch builds all of it. In `etc/Config/local.cmake`:

```
set(DJV_PYTHON ON CACHE BOOL "")
```

The feather-tk and tlRender switches follow it, the superbuild adds nanobind and builds OpenTimelineIO's Python package, and the stack is built as shared libraries so that the modules share one copy of it. The bindings compile against the Python that CMake finds, which has to be 3.12 or newer: like feather-tk's on PyPI, they are built for Python's stable ABI, so one build imports into any Python from 3.12 on. The launcher below runs them with the one CMake found.

## Running

The build writes a launcher that knows the interpreter and the module paths:

```
./build-Release/bin/djv-python render.mov
```

On Windows it is a batch file under the configuration directory, `build-Release\bin\Release\djv-python.cmd`, and it reads the libraries from the install prefix rather than the build tree: there is no rpath there, so the install has to have been run. The install gets a copy at `bin/djv-python` (`bin\djv-python.cmd` on Windows) that finds everything beside itself. The application takes an input file and the C++ application's options — see `-h` for the list. The ones useful for scripting:

- **-compare**, **-compareMode** — open a comparison "B" file and set the mode. The audio, playback, directory, and color options match the C++ application; `djv-python -h` lists them.
- **-settingsFile**, **-logFile** — the settings and log files, which default to the same directory as the C++ application: `Documents/DJV/djv-python.json` and `.log`. Pointing these elsewhere keeps a test run out of the real settings.
- **-resetSettings** — reset the settings to their defaults.
- **-listCommands**, **-command** — the command registry, the same one the C++ application has: every menu action is a named command, `-listCommands` prints them, and `-command "Playback/Seek { \"frame\": 100 }"` runs one after startup.
- **-exit** — start the user interface and exit without showing a window.
- **-screenshot** — write a screenshot of the window to a file and exit.

## The application

The application is the C++ one, bound as `djv.app.App`. `examples/python` starts it and extends it: `main.py` is the entry point, `App.py` derives from the application and adds a tool and a command, and `NotesTool.py` is the tool. The whole of it is a few dozen lines, which is the point: the menus, the tool bars, playback and the models are the application's own, so a Python program cannot drift from them, and what it adds is the only Python there is.

The application is built in steps, each a method a derived class can override to do its own work before or after the base's: `_modelsInit`, `_observersInit`, `_inputFilesInit`, `_uiInit`, and `_mainWindowInit`, in that order. The tools model and the tool widget factory exist after `_uiInit`, which is where a tool is registered so that the Tools menu and tool bar built next list it; the main window exists after `_mainWindowInit`, which is where a menu item is added.

```
class App(djv.app.App):
    def _uiInit(self):
        djv.app.App._uiInit(self)
        info = djv.models.ToolInfo()
        info.name = "Notes"
        info.icon = "Info"
        self.getToolsModel().addTool(info)
        self.getToolWidgetFactory().addTool("Notes", NotesTool)

    def _mainWindowInit(self):
        djv.app.App._mainWindowInit(self)
        self.getCommandsModel().add("Notes/Clear", "Clear the notes.", weak(self._clearNotes))
        self._clearAction = ftk.Action("Clear Notes", weak(self._clearNotes))
        self.getMainWindow().getMenuBar().getMenu("Tools").addAction(self._clearAction)
```

A tool derives from `djv.app.IToolWidget`, which is the frame every tool has — the title row and the close button — and sets its content with `setWidget`. The factory makes one each time the tool is opened, from the function it was registered with: the class itself serves, since its constructor takes what the factory passes, the context, the application, the main window and the parent. Everything the toolkit's widget events offer passes through to Python: a widget written in Python can draw, take the keyboard and mouse, and be a drag and drop source or target.

A command registered with the commands model can be run from the command line with `-command` and from a capture manifest, and the capture tool itself is the application's: `tests/capture/python.json` opens the Notes tool, types into it and runs the command, and `djv-python-example` runs that under CTest.

Everything follows the same pattern: the DJV models hold the state, and the widgets observe it. A widget changes a model through its setters, and updates itself from an observer — never directly from its own callback, so that every other view of the same state stays in step:

```
selfWeak = weakref.ref(self)
self._playerObserver = tl.PlayerObserver(
    app.observePlayer,
    lambda value: selfWeak()._playerUpdate(value))
```

Two things in that pattern matter:

- The observer object must be kept — assign it to `self`. A discarded observer disconnects.
- The callbacks capture `self` weakly. A callback handed to the C++ side is held by a `std::function`, an edge the garbage collector cannot see, so a strong capture forms a cycle that is never collected, and the settings are written when those objects are destroyed. `weak()` in `App.py` wraps a bound method for use as a callback.

Option values are copies. Reading a property, changing a field, and assigning it back is the way to change one:

```
options = filesModel.compareOptions
options.compare = tl.Compare.Wipe
filesModel.compareOptions = options
```

## Extending it {#modifying}

To add a tool, write a class deriving from `djv.app.IToolWidget` the way `NotesTool.py` does, and register it in `_uiInit` with a `ToolInfo` for the tools model — the name, the icon, where it sorts, whether it is on the tool bar, and a shortcut — and the class for the factory. To add a command, register it with the commands model in `_mainWindowInit` and add an `ftk.Action` that runs it to a menu from the menu bar. The models are reached through the application's getters, `getFilesModel()` and the rest, and the main window through `getMainWindow()`, which has the viewport, the timeline widget and the menu bar.

Smaller examples are in feather-tk's and tlRender's own `examples/python` directories, and the Python test suites — `tests/CorePyTest` and `tests/UIPyTest` in feather-tk, `tests/tlRenderPyTest` in tlRender, and `tests/djvPyTest` here — double as working usage of most of the bound API. They run with `python -m unittest discover tests`, or through CTest as `feather_tk-test`, `tlrender-test`, and `djv-python-test`.
